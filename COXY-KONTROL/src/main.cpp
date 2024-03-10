#include "NETWORK.h"   
#include <Arduino.h>
#include "ESPAsyncWebServer.h"
#include <WebSocketsServer.h>  
#include <ArduinoJson.h>    
#include <Preferences.h>
#include <SPIFFS.h>
#include <Stepper.h>
#include <Wire.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define INLED 15 // LOLIN S2 MINI ONBOARD LED

#define SDA_PIN 21  //  custom I2C SDA pin
#define SCL_PIN 34  //  custom I2C SCL pin

#define BME680_ // comment if BME680 sensor is not used
#ifdef BME680_ 
  Adafruit_BME680 bme; // BME680 sensor I2C
#endif

#define DALLAS_ // comment if  DS18B20 DALLAS TEMP SENSOR sensor is not used
#ifdef DALLAS_ 
  #define ONE_WIRE_BUS 12 
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature dallas(&oneWire); // create DS18B20 sensor instance
#endif

#define SENSIRION_ // comment if SCD41x sensor is not used
#ifdef SENSIRION_
  SensirionI2CScd4x scd4x; // create sensirion sensor instance
  // https://github.com/Sensirion/arduino-i2c-scd4x
  uint16_t alt = 9; // AMSTERDAM // local altitude in meters above sea level
  uint32_t pressureCalibration;
  float tempOffset = 0.1;
  bool ascSetting = false; // Turn automatic self calibration off
#endif

#define OLED_ // comment if SSD1306 OLED displayis not used
#ifdef OLED_ 
  #define OLED_RESET 17
  #define SCREEN_WIDTH 128 
  #define SCREEN_HEIGHT 64 
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

#define NEOPIXEL_ // comment if neopixels are not used
#ifdef NEOPIXEL_
  #include <Adafruit_NeoPixel.h>
  const int neoPixelPin = 7;  
  Adafruit_NeoPixel neopixel(1, neoPixelPin, NEO_GRB + NEO_KHZ800);
#endif


#define MOTOR_
#ifdef MOTOR_
  const int stepsPerRevolution = 600;  // number of steps per revolution
  const int endstopPin = 9;  // endstop pin
  // ULN2003 Motor Driver Pins
  #define IN1 35 
  #define IN2 33 
  #define IN3 18 
  #define IN4 16 
  Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);
#endif


// sensors' global variables
uint16_t CO2 = 0;
float TEMP = 0;
float HUM = 0;
float DSTEMP = 0;
float BMETEMP = 0;
float BMEHUM = 0;
float BMEPRES= 0;
float BMEGAS= 0;

TaskHandle_t countdownTaskHandle = NULL; // countdown and sensors read timeout handler

// SPIFFS memory global variables
long int SPIFFS_TOTAL;
long int SPIFFS_USED;
long int SPIFFS_REMAINING;

// variables via websocket 
int endStop = 0; // cap is down in place
int countdown = 0; // counter for visualizing read sensors timeout 


// millis used for resetting a timestamp for the datalog json file
unsigned long startupTime;
unsigned long adjustedMillis;

Preferences preferences; // unused for esp32 internal memory prefs

// init instances of web/socket servers
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);   


/////////////////////////////////
void printUint16Hex(uint16_t value) {
    Serial.print(value < 4096 ? "0" : "");
    Serial.print(value < 256 ? "0" : "");
    Serial.print(value < 16 ? "0" : "");
    Serial.print(value, HEX);
}

/////////////////////////////////
void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    Serial.print("Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    Serial.println();
}

/////////////////////////////////
void sendJson(String l_type, String l_value) {
    String jsonString = "";
    StaticJsonDocument<200> doc;
    JsonObject object = doc.to<JsonObject>();
    object["type"] = l_type;              
    object["value"] = l_value;
    serializeJson(doc, jsonString);           
    webSocket.broadcastTXT(jsonString);
}

/////////////////////////////////
void read_prefs(){
  preferences.begin("vimo-pref", false); 
  unsigned int mot_speed = preferences.getUInt("mot_speed", 0);
  preferences.end();
}

/////////////////////////////////
void write_prefs(){
  preferences.begin("vimo-pref", false); 
  preferences.end();
}


///////////////////////////////////
void cap_Open(){
  #ifdef MOTOR_
    myStepper.step(stepsPerRevolution / 2);
  #endif
}

///////////////////////////////////
void cap_Close(){
#ifdef MOTOR_
  int count = 1;
  myStepper.step(-1*(stepsPerRevolution / 2));

  // this is useful when using an endstop 
  // while(1){
  //       // Serial.println("Signal wire is not connected to ground.");
  //       myStepper.step(-1);
  //       count++;
  //         // myStepper.step(-stepsPerRevolution);
  //       if(digitalRead(endstopPin) == LOW){ 
  //           delay(2000);
  //           return;
  //       }
  //   }
#endif

}

///////////////////////////////////
void readBME680(){

  #ifdef BME680_
      if (bme.performReading()) {
      BMETEMP = bme.temperature;
      Serial.print("Temperature: ");
      Serial.print(BMETEMP);
      Serial.println(" *C");
    
      BMEPRES = bme.pressure / 100.0;
      Serial.print("Pressure: ");
      Serial.print(BMEPRES);
      Serial.println(" hPa");
      
      BMEHUM = bme.humidity;
      Serial.print("Humidity: ");
      Serial.print(BMEHUM);
      Serial.println(" %");

      BMEGAS = bme.gas_resistance / 1000.0;
      Serial.print("Gas: ");
      Serial.print(BMEGAS);
      Serial.println(" KOhms");

      Serial.println();
    } else {
      Serial.println("Failed to read data from BME680 sensor!");
    }
  #endif

}
////////////////////////////
void dataLog(char* varname, float variable, unsigned long timestamp) {

unsigned long currentTime = timestamp;
unsigned long seconds = currentTime / 1000;
unsigned long minutes = seconds / 60;
unsigned long hours = minutes / 60;

seconds %= 60;
minutes %= 60;

String myTimeString = String(hours) + ":" + String(minutes) + ":" + String(seconds);


  if(SPIFFS_REMAINING > 20000){ // check if enough memory is left in SPIFFS (min 20k)
 
      // Open JSON log file in append mode
      File logFile = SPIFFS.open("/datalog.json", FILE_APPEND);
      if (!logFile) {
        Serial.println("Failed to open log file");
        return;
      }

      // Create JSON object
      StaticJsonDocument<100> jsonDoc;
      jsonDoc[varname] = variable;
      jsonDoc["timestamp"] = myTimeString;

      // Serialize JSON object to string
      String jsonString;
      serializeJson(jsonDoc, jsonString);

      // Append JSON string to log file
      logFile.println(jsonString);

      // Close log file
      logFile.close();

      String datastring = String(varname) + ": " + String(variable) + " " + myTimeString;
      sendJson("_COXY_DATALOG",  datastring);

   }

}

///////////////////////////////////
void SensiLog(){
      // logging SENSIRION data to datalog.json file in SPIFFS memory
      adjustedMillis = millis() - startupTime;

      unsigned long time_stamp = adjustedMillis; // using simple millis as timestamp

      dataLog("Co2",CO2, time_stamp);
      delay(100);
      dataLog("TEMP",TEMP, time_stamp);
      delay(100);
      dataLog("HUM",HUM, time_stamp);
      delay(100);
}


/////////////////////////////////
void updateWebClient(){
 
  String l_value = "";

  // WIFI SSID  
  l_value = current_SSID;
  sendJson("_SSID",  l_value);

  // WIFI IP  
  l_value = current_IP;
  sendJson("_IP",  l_value);

  #ifdef SENSIRION_
    // sensirion data
    l_value = String(CO2);
    sendJson("_CO2",  l_value);
    l_value = String(TEMP);
    sendJson("_TEMP",  l_value);
    l_value = String(HUM);
    sendJson("_HUM",  l_value);
  #endif

  #ifdef BME680_
    // bme data
    l_value = String(BMEPRES);
    sendJson("_BMEPRES",  l_value);
    l_value = String(BMETEMP);
    sendJson("_BMETEMP",  l_value);
    l_value = String(BMEHUM);
    sendJson("_BMEHUM",  l_value);
    l_value = String(BMEGAS);
    sendJson("_BMEGAS",  l_value);
  #endif

  #ifdef DALLAS_
    // DS data
    l_value = String(DSTEMP);
    sendJson("_DSTEMP",  l_value);
  #endif

  // SPIFF TOTAL memory
  l_value = String(SPIFFS_TOTAL);
  sendJson("_SPIFFS_TOTAL",  l_value);
  // SPIFF USED memory
  l_value = String(SPIFFS_USED);
  sendJson("_SPIFFS_USED",  l_value);
  // SPIFF REMAINING memory
  l_value = String(SPIFFS_REMAINING);
  sendJson("_SPIFFS_REMAINING",  l_value);

}

///////////////////////////////////
void readDallas(){
  #ifdef DALLAS_
    dallas.requestTemperatures();
    DSTEMP = dallas.getTempCByIndex(0);
  #endif
}

///////////////////////////////////
void readSensirion(){

  #ifdef SENSIRION_
    uint16_t error;
    char errorMessage[256];

    delay(100);

    // Read Measurement
    CO2= 0;
    TEMP = 0.0f;
    HUM = 0.0f;
    bool isDataReady = false;
    error = scd4x.getDataReadyFlag(isDataReady);
    if (error) {
        Serial.print("Error trying to execute getDataReadyFlag(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
         return;
    }
    if (!isDataReady) {
         return;
    }
    error = scd4x.readMeasurement(CO2, TEMP, HUM);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (CO2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
    } else {
        Serial.print("Co2:");
        Serial.print(CO2);
        Serial.print("\t");
        Serial.print("Temperature:");
        Serial.print(TEMP);
        Serial.print("\t");
        Serial.print("Humidity:");
        Serial.println(HUM);
    }

   updateWebClient();
   #endif

}


/////////////////////////////////
void updateOLED(){

  #ifdef OLED_
    String line1 = "Co2: " + String(CO2);
    String line2 = "Temperature: " + String(TEMP);
    String line3 = "Humidity: " + String(HUM);
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("SENSIRION SCD41");
    display.setCursor(0, 20);
    display.println(line1);
    display.setCursor(0, 35);
    display.println(line2);
    display.setCursor(0, 50);
    display.println(line3);
    display.display();
  #endif

}


/////////////////////////////////
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      
  switch (type) {                                 
    
    case WStype_DISCONNECTED:  // client disconnected
      Serial.println("Client " + String(num) + " disconnected");
      // Attempt to reconnect after a short delay
      delay(2000);
      webSocket.begin();  // restart websocket
      Serial.println("Websocket server re-started");
      break;
   
    case WStype_CONNECTED:  // client connected                         
      Serial.println("Client " + String(num) + " connected");
      updateWebClient(); // update all connected clients
      break;
      
    case WStype_ERROR:
      Serial.println("WebSocket error");
      // Attempt to reconnect after a short delay
      delay(2000);
      // reSTART WEBSOCKET 
      webSocket.begin();   // restart websocket   
      Serial.println("Websocket server re-started");
      break;

    case WStype_TEXT: // if a client has sent data, do JSON decoding
      StaticJsonDocument<200> doc; 
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        const char* l_type = doc["type"];
        const int l_value = doc["value"];
        Serial.println("Type: " + String(l_type));
        Serial.println("Value: " + String(l_value));

        if(String(l_type) == "CAP_motor") {
          int motor_action = int(l_value);
          sendJson("CAP_open", String(motor_action));
          if ( motor_action == 0){
              cap_Close();
          }else if ( motor_action == 1){
              cap_Open();
          }    
        }
        
        if(String(l_type) == "REFRESH_gui") {
          updateWebClient();
        }
        

        if(String(l_type) == "SAVE_prefs") {
          write_prefs();
        }

        if(String(l_type) == "TIME_stamp") {
          // Serial.println(l_value);
          String myTS  =  String(l_value);
          Serial.print("TIMESTAMP: ");
          Serial.println(myTS);
        }

      }

      Serial.println("");
      break;
  }
}




/////////////////////////////////
void check_SPIFFS(){

  // Calculate total space
  SPIFFS_TOTAL = SPIFFS.totalBytes();
  Serial.print("Total space: ");
  Serial.print(SPIFFS_TOTAL);
  Serial.println(" bytes");

  // Calculate used space
  SPIFFS_USED = SPIFFS.usedBytes();
  Serial.print("Used space: ");
  Serial.print(SPIFFS_USED);
  Serial.println(" bytes");

  // Calculate free space
  SPIFFS_REMAINING = SPIFFS_TOTAL - SPIFFS_USED;
  Serial.print("Free space: ");
  Serial.print(SPIFFS_REMAINING);
  Serial.println(" bytes");
  
  String l_value = "";

  // SPIFF TOTAL memory
  l_value = String(SPIFFS_TOTAL);
  sendJson("_SPIFFS_TOTAL",  l_value);
  // SPIFF USED memory
  l_value = String(SPIFFS_USED);
  sendJson("_SPIFFS_USED",  l_value);
  // SPIFF REMAINING memory
  l_value = String(SPIFFS_REMAINING);
  sendJson("_SPIFFS_REMAINING",  l_value);

}

////////////////////////////
void list_SPIFFS(){  
  // Open the directory
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  // Create a file to store the file list
  File fileList = SPIFFS.open("/fileList.txt", FILE_WRITE);

  if (!fileList) {
    Serial.println("Failed to create fileList.txt");
    return;
  }

  while (file) {
    // Collect file names and sizes and write to the fileList.txt
    fileList.print(file.name());
    fileList.print(" ");
    fileList.println(file.size());
    Serial.print("Name: ");
    Serial.print(file.name());
    Serial.print(", Size: ");
    Serial.println(file.size());
    file = root.openNextFile();
  }
  // Close the fileList.txt
  fileList.close();
  Serial.println("File list generation completed");
  sendJson("_SPIFFS_FILELIST",  "");
  file.close(); 
  root.close();
}

////////////////////////////
void initDataLog(){
    // Remove the existing data.json file
  if (SPIFFS.exists("/datalog.json")) {
    SPIFFS.remove("/datalog.json");
    Serial.println("Removed existing datalog.json file");
  }

  // Create a new data.json file
  File file = SPIFFS.open("/datalog.json", "w");
  if (!file) {
    Serial.println("Failed to create datalog.json");
    return;
  }else{
     Serial.println("Created new datalog.json file");
  }
  file.close();
}

//////////////////////////////
void updateWIFI(){
  sendJson("_SSID",  current_SSID);
  sendJson("_IP",  current_IP);
}

///////////////////////////////////
void countdownTask(void *arg) {
  while(1){

      if(countdown >= 0){
        countdown--; // Decrement the countdown value
        String l_value = ""; 
        l_value = String(countdown);
        sendJson("_COUNTDOWN",  l_value);
      }else{
        countdown = 30;

      updateWIFI();
      check_SPIFFS();
      list_SPIFFS();
      SensiLog();

      #ifdef SENSIRION_
        readSensirion();
      #endif
       
      #ifdef OLED_ 
        updateOLED();
      #endif 

      #ifdef BME680_
        readBME680();
      #endif 

      #ifdef DALLAS_
         readDallas();
      #endif  

      }
  
    long int intervallo = 1 * 1000; // 1 second interval
    vTaskDelay(intervallo / portTICK_RATE_MS);
  }

}

///////////////////////////////////
void setup() {

  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  startupTime = millis(); // Store the startup time

   #ifdef MOTOR_
    pinMode(endstopPin, INPUT_PULLUP); // end_switch pin
   #endif


 // Connect to a known WiFi network if available
  connectToWiFi();
  // If not connected, create an Access Point
  if (WiFi.status() != WL_CONNECTED) {
    createAccessPoint();
  }else{
    #ifdef NEOPIXEL_
    for(int i=1; i<=3; i++){
      neopixel.setPixelColor(0, neopixel.Color(0, 0, 200)); 
      neopixel.show();
      delay(250);
      neopixel.setPixelColor(0, neopixel.Color(0, 0, 0)); 
      neopixel.show();
      delay(250);
      #endif
    }
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/index.html", "text/html");
    });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/favicon.ico", "image/x-icon");
  });

  server.on("/coxy.js", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/coxy.js", "text/javascript");
  });

  server.on("/highcharts.js", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/highcharts.js", "text/javascript");
  });

  server.on("/fileList.txt", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/fileList.txt", "text/plain");
  });


  server.on("/lighting.png", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/lighting.png", "image/png");
  });

  server.on("/transpix.png", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/transpix.png", "image/png");
  });

  server.on("/trilogoz.png", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/trilogoz.png", "image/png");
  });

  server.on("/glassdisplay.png", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/glassdisplay.png", "image/png");
  });
  
  server.on("/origami.ttf", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/origami.ttf", "font/ttf");
  });

  server.on("/cons.ttf", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/cons.ttf", "font/ttf");
  });

    server.on("/datalog.json", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/datalog.json", "application/json");
  });


  // Start server
  server.begin();
  Serial.println("HTTP server started");

  // START WEBSOCKET 
  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent);                  // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"
  Serial.println("Websocket server started");

  // START I2C communication 
  Wire.begin(SDA_PIN, SCL_PIN); 

  // INIT NEOPIXEL 
  #ifdef NEOPIXEL_
    neopixel.setPixelColor(0, neopixel.Color(200, 0, 0)); 
    neopixel.show();
  #endif

 // INIT MOTOR 
 #ifdef MOTOR_
   myStepper.setSpeed(10); 
 #endif

 // INIT DS18B20 TEMP SENSOR 
  #ifdef DALLAS_
    dallas.begin(); // DS18B20 temp. sensor begin
  #endif

// INIT SENSIRION SCD4x SENSOR 
 #ifdef SENSIRION_
     uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire); // Initialize SENSIRION sensor 
  
    scd4x.setSensorAltitude(alt); //  altitude in meters above sea level.
    //scd4x.setAmbientPressure(pressureCalibration); // 
    scd4x.setTemperatureOffset(tempOffset);
    scd4x.setAutomaticSelfCalibration(ascSetting);
    
    uint16_t ascEnabled;
    error = scd4x.getAutomaticSelfCalibration(ascEnabled);
    if (error) {
        Serial.print("Error trying: ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("AutoSelfCalibrations is: ");
        if (ascEnabled) Serial.println("ON");
        if (!ascEnabled) Serial.println("OFF");
    }
    
    uint16_t setAlt;
    error = scd4x.getSensorAltitude(setAlt);
    if (error) {
        Serial.print("Error trying: ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("Altitude is set to: ");
        Serial.println(setAlt);
    }

    float settempOffset;
    error = scd4x.getTemperatureOffset(settempOffset);
    if (error) {
        Serial.print("Error trying: ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("Temperature Offset set to: ");
        Serial.println(settempOffset);
    }
    

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
      if (error) {
          Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
          errorToString(error, errorMessage, 256);
          Serial.println(errorMessage);
      }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
      } else {
          printSerialNumber(serial0, serial1, serial2);
      }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
      if (error) {
          Serial.print("Error trying to execute startPeriodicMeasurement(): ");
          errorToString(error, errorMessage, 256);
          Serial.println(errorMessage);
      }

    Serial.println("Waiting for first measurement... (5 sec)");

    delay(5000);

      #ifdef NEOPIXEL_
      neopixel.setPixelColor(0, neopixel.Color(200, 200, 0)); 
      neopixel.show();
      #endif

     

#endif

// INIT BME680 sensor
#ifdef BME680_
    if (!bme.begin()) {
      Serial.println("Could not find BME680 sensor!");
      while (1);
    }
    Serial.println("BME680 sensor found!");
    readBME680();
#endif


#ifdef OLED_
  // Initialize the SSD1306 OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
    Serial.println(F("SSD1306 display allocation failed"));
    for(;;);
  }  
  display.clearDisplay(); // Clear the display buffer
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Hello, world!");
  display.display();  // Display the content on the screen
#endif

  // Init SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }else{
    initDataLog();
    Serial.println("SPIFFS mounted and datalog initiated successfully");
  }
  
  Serial.println("checking SPIFFS memory and files");
  check_SPIFFS();
  delay(500);
  list_SPIFFS();
  delay(500);

#ifdef NEOPIXEL_
  neopixel.setPixelColor(0, neopixel.Color(0, 200, 0)); 
  neopixel.show();
#endif

  // START TASK FOR COUNTDOWN AND SENSORS READING
   xTaskCreate(countdownTask, "Countdown Task", 2048, NULL, 1, &countdownTaskHandle);

}

///////////////////////////////////
void loop() {
  webSocket.loop(); 
}

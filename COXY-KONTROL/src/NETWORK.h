#include <WiFi.h>
#include <ESPmDNS.h>
// definitions of your desired intranet created by the ESP32
// IPAddress PageIP(192, 168, 1, 1);
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnet(255, 255, 255, 0);

// SSID and password of Wifi connection:
const char *ssid = "YOUR_WIFI_SSID";  // your WiFi SSID
const char *password = "YOUR_WIFI_PASSWORD"; // your WiFi password
const char *apHostname = "coxy";  // Replace with your desired hostname
const IPAddress apIP(192, 168, 23, 1);  // Replace with your desired IP address
String current_IP;
String current_SSID = "NOT_CONNECTED";

//////////////////////////////////////////////
void printWifiStatus() {
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
//////////////////////////////////////////////
void connectToWiFi() {

  WiFi.disconnect(true);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);    
  WiFi.softAPConfig(INADDR_NONE, INADDR_NONE, INADDR_NONE);   
  WiFi.setHostname(apHostname);
  WiFi.mode(WIFI_STA);


  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {

    for(int i=1; i<=3; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
  }

    WiFi.setHostname(apHostname); // assign custom hostname (e.g. "coxy")

  // Initialize mDNS
  if (MDNS.begin(apHostname)) {
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
     // ESP.restart(); // Restart ESP, optional if you keep encountering connection errors
  }


  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print where to go in a browser:
  Serial.print("Open http://");
  Serial.println(WiFi.localIP());
  Serial.println("You can point your browser to: http://" + String(apHostname) + ".local");
  
  current_SSID = WiFi.SSID();

  IPAddress ip = WiFi.localIP();
  current_IP = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  Serial.println("current_IP: " + String(current_IP));

  printWifiStatus();

}

//////////////////////////////////////////////
void createAccessPoint() {

  WiFi.mode(WIFI_AP);

  Serial.println("Creating Access Point...");

  // Generate a unique SSID based on ESP32 MAC address
  // String apSSID = "ESP32-" + WiFi.macAddress();
  String apSSID = "COXY-NET";

  WiFi.softAPConfig(INADDR_NONE, INADDR_NONE, INADDR_NONE);   
  // Set the hostname for the Access Point
  WiFi.softAPsetHostname(apHostname);
  // Set a specific IP configuration for the Access Point
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

 // Start the Access Point
  WiFi.softAP(apSSID.c_str());

  current_SSID = apSSID;

  // Initialize mDNS
  if (MDNS.begin(apHostname)) {
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

  Serial.println("Local Access Point created");
  Serial.print("SSID: ");
  Serial.println(apSSID);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  // WiFi.softAPsetHostname(apHostname);
  Serial.print("Hostname: ");
  Serial.println(apHostname);

  // print where to go in a browser:
  Serial.print("Open http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("You can point your browser to: http://" + String(apHostname) + ".local");

  IPAddress thisIP = WiFi.softAPIP(); 
  current_IP =String(thisIP[0]) + "." + String(thisIP[1]) + "." + String(thisIP[2]) + "." + String(thisIP[3]);

  printWifiStatus();
}




///////////// CoXY manager 1.0 / TeZ ~ 2024 ////////////////
var SSID = ":::";
var IP = ":::";
var socket;
var CO2 = 0;
var TEMP = 0;
var HUM = 0;
var DSTEMP = 0;
var BMETEMP = 0;
var BMEHUM = 0;
var BMEPRES = 0;
var BMEGAS= 0;
var chartC ;
var chartT ;
var chartH ;
var CapStatus = 0; 
var SPIFFS_TOTAL = 0;
var SPIFFS_USED = 0;
var SPIFFS_REMAINING = 0;

var countdown = 0;

document.addEventListener("DOMContentLoaded", function() {

  init();

   chartC = new Highcharts.Chart({

    global: {
      useUTC: true
      },

    chart:{ 
      renderTo : 'coxy-co2', 
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'rgba(0,0,0,200)',  
      borderRadius: 20,
      borderWidth: 2,
      type: 'line',
      // plotBackgroundImage: '/tardibg.jpg'
    },
    title: { 
      text: '~',
      style:{"fontSize": "1.0em", "fontWeight": "bold", "color":"#aaa"}
    },
    series: [{
      showInLegend: false,
      data: []
    }],
    plotOptions: {
      line: { animation: false,
        dataLabels: { enabled: true }
      },
      series: { color: '#F8BBD0' }
    },
    xAxis: { type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S'},
      labels: {
        style: {
            color: '#fff' 
          }
        }
    },
    yAxis: {
      title: { text: 'CO2 level', style: {
        color: '#fff' } },
      gridLineDashStyle: 'longdash',
      labels: {
        style: {
            color: '#fff' 
          }
        }
    },
    credits: { enabled: false },
    labels: {
      style: {
          color: '#fff' 
        }
      }
  });




 chartT = new Highcharts.Chart({

  global: {
    useUTC: true
    },

  chart:{ 
    renderTo : 'coxy-temp', 
    backgroundColor: 'rgba(0,0,0,0)',
    borderColor: 'rgba(0,0,0,200)',  
    borderRadius: 20,
    borderWidth: 2,
    type: 'line',
    // plotBackgroundImage: '/tardibg.jpg'
  },
  title: { 
    text: '~',
    style:{"fontSize": "1.0em", "fontWeight": "bold", "color":"#aaa"}
  },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#03fcec' }  //#F8BBD0
  },
  xAxis: { type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S'},
    labels: {
      style: {
          color: '#fff' 
        }
      }
  },
  yAxis: {
    title: { text: 'Temperature (c)', style: {
      color: '#fff' } },
    gridLineDashStyle: 'longdash',
    labels: {
      style: {
          color: '#fff' 
        }
      }
  },
  credits: { enabled: false },
  labels: {
    style: {
        color: '#fff' 
      }
    }
  });

  chartH = new Highcharts.Chart({

    global: {
      useUTC: true
      },
  
    chart:{ 
      renderTo : 'coxy-hum', 
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'rgba(0,0,0,200)',  
      borderRadius: 20,
      borderWidth: 2,
      type: 'line',
      // plotBackgroundImage: '/tardibg.jpg'
    },
    title: { 
      text: '~',
      style:{"fontSize": "1.0em", "fontWeight": "bold", "color":"#aaa"}
    },
    series: [{
      showInLegend: false,
      data: []
    }],
    plotOptions: {
      line: { animation: false,
        dataLabels: { enabled: true }
      },
      series: { color: '#6f0260' }
    },
    xAxis: { type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S'},
      labels: {
        style: {
            color: '#fff' 
          }
        }
    },
    yAxis: {
      title: { text: 'Humidity %', style: {
        color: '#fff' } },
      gridLineDashStyle: 'longdash',
      labels: {
        style: {
            color: '#fff' 
          }
        }
    },
    credits: { enabled: false },
    labels: {
      style: {
          color: '#fff' 
        }
      }
    });


});


///////////////////////
function init() {

  connectWebSocket();

}

///////////////////////
function connectWebSocket() {
  socket = new WebSocket('ws://' + window.location.hostname + ':81/');

  socket.onopen = function(event) {
    console.log('WebSocket connection established');
    const ssid_span = document.getElementById('_SSID');
    ssid_span.style.backgroundColor = "#00ff00";
    const ip_span = document.getElementById('_IP');
    ip_span.style.backgroundColor = "#00ff00";
  };

  socket.onmessage = function(event) {
    processCommand(event);
  };

  socket.onclose = function(event) {
    console.log('WebSocket connection closed');
    // Attempt to reconnect after a short delay
    setTimeout(connectWebSocket, 2000);
  };

  socket.onerror = function(error) {
    console.error('WebSocket error:', error);
    // Attempt to reconnect after a short delay
    setTimeout(connectWebSocket, 2000);
    const ssid_span = document.getElementById('_SSID');
    ssid_span.style.backgroundColor = "#ff0000";
    const ip_span = document.getElementById('_IP');
    ip_span.style.backgroundColor = "#ff0000";
  };

  return socket;
}



///////////////////////
function changeCap(){

  var buttonSpan = document.getElementById("motorbutton");
  
    if(CapStatus){
    buttonSpan.innerHTML = 'OPEN';
    buttonSpan.style.backgroundColor = '#0f0';
    buttonSpan.style.color = '#000';
  }else{
    buttonSpan.innerHTML = 'CLOSE';
    buttonSpan.style.backgroundColor = '#000';
    buttonSpan.style.color = '#0f0';
  }

  CapStatus=!CapStatus;
  console.log(CapStatus);
  var msg = { type: 'CAP_motor', value:CapStatus}
  socket.send(JSON.stringify(msg)); 

}

///////////////////////
function processCommand(event) {
  var obj = JSON.parse(event.data);
  var type = obj.type;

  if (type.localeCompare('_SSID') == 0) { 
    SSID = String(obj.value); 
     console.log("_SSID: " + SSID); 
     update_SSID();
    }

   else if (type.localeCompare('_IP') == 0) { 
    IP = String(obj.value); 
     console.log("_IP: " + IP); 
     update_IP();
    }
      
  else if (type.localeCompare('_ENDSTOP') == 0) { 
  var _endstop= parseInt(obj.value); 
   console.log("_endstop received: " + _endstop); 
  }
  
  else if (type.localeCompare('AUTOSTART') == 0) { 
    AUTOSTART = parseInt(obj.value); 
      set_autostart();
  }
  
  else if (type.localeCompare('_CO2') == 0) { 
    CO2 = parseFloat(obj.value); 
    graph_co2();
    console.log("_CO2: " + CO2);  
  }
  else if (type.localeCompare('_TEMP') == 0) { 
    TEMP = parseFloat(obj.value); 
    graph_Temp();
    update_Temp();
    console.log("_TEMP: " + TEMP);  
  }
  else if (type.localeCompare('_HUM') == 0) { 
    HUM = parseFloat(obj.value); 
    graph_Hum();
    update_Hum();
    console.log("_HUM: " + HUM);  
  }
  else if (type.localeCompare('_DSTEMP') == 0) { 
    DSTEMP = parseFloat(obj.value); 
    update_DSTemp();
    console.log("_DSTEMP: " + DSTEMP);  
  }
  else if (type.localeCompare('_BMETEMP') == 0) { 
   BMETEMP = parseFloat(obj.value); 
    update_BMETemp();
    console.log("_BMETEMP: " + BMETEMP);  
  }
  else if (type.localeCompare('_BMEHUM') == 0) { 
    BMEHUM = parseFloat(obj.value); 
     update_BMEHum();
     console.log("_BMEHUM: " + BMEHUM);  
   }
  else if (type.localeCompare('_BMEPRES') == 0) { 
    BMEPRES = parseFloat(obj.value); 
     update_BMEPres();
     console.log("_BMEPRES: " + BMEPRES);  
   }
   else if (type.localeCompare('_BMEGAS') == 0) { 
    BMEGAS = parseFloat(obj.value); 
     update_BMEGas();
     console.log("_BMEGAS: " + BMEGAS);  
   }
   else if (type.localeCompare('_SPIFFS_TOTAL') == 0) { 
    SPIFFS_TOTAL = parseInt(obj.value); 
    updateSPIFFS();
    console.log("_SPIFF_TOTAL: " + SPIFFS_TOTAL);  
    updateSPIFFS();
   }
   else if (type.localeCompare('_SPIFFS_USED') == 0) { 
    SPIFFS_USED = parseInt(obj.value); 
    updateSPIFFS();
    console.log("_SPIFFS_USED: " + SPIFFS_USED);  
    updateSPIFFS();
   }
   else if (type.localeCompare('_SPIFFS_REMAINING') == 0) { 
    SPIFFS_REMAINING = parseInt(obj.value); 
    updateSPIFFS();
    console.log("_SPIFFS_REMAINING: " + SPIFFS_REMAINING);  
    updateSPIFFS();
   }
   else if (type.localeCompare('_SPIFFS_FILELIST') == 0) { 
    updateSPIFFSlist();
    const fileListDiv = document.getElementById('spiffs_folder');
    fileListDiv.classList.add('spiffs_folder');
   }
   else if (type.localeCompare('_COXY_DATALOG') == 0) { 
    var myLine = obj.value; // already a string
    console.log(myLine);  
    updateConsol(myLine);
   }
   else if (type.localeCompare('_COUNTDOWN') == 0) { 
    countdown = parseInt(obj.value);
    updateProgressBar((countdown * 100) / 30);
   }

}

///////////////////////
function updateProgressBar(progress) {
  const progressBar = document.getElementById('progress-bar');
  progressBar.style.width = progress + '%';
}


///////////////////////
function downloadJsonFile() {

  const filename = 'datalog.json';

  // Fetch the JSON file from the server
  fetch(filename)
    .then(response => response.blob())
    .then(blob => {
      // Create a temporary anchor element
      const link = document.createElement('a');
      link.href = window.URL.createObjectURL(blob);
      link.download = filename;

      // Trigger the download
      link.click();

      // Clean up
      window.URL.revokeObjectURL(link.href);
    })
    .catch(error => console.error('Error downloading JSON file:', error));
}


///////////////////////
function updateSPIFFSlist(){

  // Fetch the file list from ESP32 server
  fetch('/fileList.txt')
    .then(response => response.text())
    .then(data => {
      const fileListDiv = document.getElementById('spiffs_folder');
      fileListDiv.classList.add('spiffs-folder');
      fileListDiv.innerHTML = 'root folder';

      // Split the data into lines
      const lines = data.split('\n');

      // Process each line (file name and size)
      lines.forEach(line => {
        // Split each line into file name and size
        const [fileName, fileSize] = line.split(' ');

        // Create a paragraph element with file name and size
        const fileElement = document.createElement('p');
        fileElement.textContent = `${fileName} - ${fileSize} bytes`;
        fileElement.classList.add('spiffs-list'); 
        fileListDiv.appendChild(fileElement);
      });
    })
    .catch(error => console.error('Error fetching file list:', error));
}
///////////////////////
function graph_co2(){  
    var x = (new Date()).getTime(),
        y = CO2;
    if(chartC.series[0].data.length > 40) {
      chartC.series[0].addPoint([x, y], true, true, true);
    } else {
      chartC.series[0].addPoint([x, y], true, false, true);
    }
    var spanElement = document.getElementById("data_co2");
    spanElement.innerHTML = String(CO2);
}

///////////////////////
function graph_Temp(){  
  var x = (new Date()).getTime(),
      y = TEMP;
  if(chartT.series[0].data.length > 40) {
    chartT.series[0].addPoint([x, y], true, true, true);
  } else {
    chartT.series[0].addPoint([x, y], true, false, true);
  }
  var spanElement = document.getElementById("data_temp");
  spanElement.innerHTML = String(TEMP);
}

///////////////////////
function graph_Hum(){  
  var x = (new Date()).getTime(),
      y = HUM;
  if(chartH.series[0].data.length > 40) {
    chartH.series[0].addPoint([x, y], true, true, true);
  } else {
    chartH.series[0].addPoint([x, y], true, false, true);
  }
  var spanElement = document.getElementById("data_hum");
  spanElement.innerHTML = String(HUM);
}
///////////////////////
function update_Temp(){  
  var spanElement = document.getElementById("data_temp");
  spanElement.innerHTML = String(TEMP) + "c";
}
///////////////////////
function update_Hum(){  
  var spanElement = document.getElementById("data_hum");
  spanElement.innerHTML = String(HUM) + "%";
}
///////////////////////
function update_DSTemp(){  
  var spanElement = document.getElementById("data_dstemp");
  spanElement.innerHTML = String(DSTEMP) + "c";
}

///////////////////////
function update_BMETemp(){  
  var spanElement = document.getElementById("data_bmetemp");
  spanElement.innerHTML = String(BMETEMP) + "c";
}
///////////////////////
function update_BMEHum(){  
  var spanElement = document.getElementById("data_bmehum");
  spanElement.innerHTML = String(BMEHUM) + "%";
}
///////////////////////
function update_BMEPres(){  
  var spanElement = document.getElementById("data_bmepres");
  spanElement.innerHTML = String(BMEPRES) + "hPa";
}
///////////////////////
function update_BMEGas(){  
  var spanElement = document.getElementById("data_bmegas");
  spanElement.innerHTML = String(BMEGAS) + "KOhm";
}
///////////////////////
function updateSPIFFS(){
  var divElement = document.getElementById("spiffs_data");
  divElement.innerHTML=  
  "TOTAL: " + String(SPIFFS_TOTAL / 1000) + "Kb" + "<br>" + 
  "USED: " + String(SPIFFS_USED  / 1000) + "Kb"  + "<br>" +
  "REMAIN.: " + String(SPIFFS_REMAINING  / 1000) + "Kb";
}
///////////////////////
function updateConsol(text) {
  const consoleField = document.getElementById('consol');
  // Append new text to the existing value
  consoleField.value += text + '\n';
  // Scroll to the bottom to always show the latest text
  consoleField.scrollTop = consoleField.scrollHeight;
}
///////////////////////
function update_SSID(){
  var divElement = document.getElementById("_SSID");
  divElement.innerHTML=  String(SSID);
}
///////////////////////
function update_IP(){
  var divElement = document.getElementById("_IP");
  divElement.innerHTML=  String(IP);
}
///////////////////////
function getTimeString(){
    // Create a new Date object
  const currentDate = new Date();
  // Get the current date and time components
  const year = currentDate.getFullYear(); // Get the year (4 digits)
  const month = currentDate.getMonth() + 1; // Get the month (0-11, add 1 to get 1-12)
  const day = currentDate.getDate(); // Get the day of the month (1-31)
  const hours = currentDate.getHours(); // Get the hours (0-23)
  const minutes = currentDate.getMinutes(); // Get the minutes (0-59)
  const seconds = currentDate.getSeconds(); // Get the seconds (0-59)
  // Output the current date and time
  var myString = year+"_"+month+"_"+day+" "+hours+":"+minutes+":"+seconds;
  return myString
}
///////////////////////
function ShowHide(thisLayer){
  var myLayer = document.getElementById(thisLayer);

  if (myLayer.style.display == 'none') {
      // If the layer is hidden, show it
      myLayer.style.display = 'block';
    } else {
      // If the layer is visible, hide it
      myLayer.style.display = 'none';
    }
}
///////////////////////
window.onload = function(event) {
  // init();
  console.log('window.onload ok');

  updateSPIFFS();
  updateSPIFFSlist();
  update_SSID();
  update_IP();

  document.getElementById("spiffs_layer").addEventListener('click',function(event) {
    var myLayer = document.getElementById('spiffs_layer');
    if(myLayer.style.display == "block"){
      myLayer.style.display = "none";
    }else{
      myLayer.style.display = "block";
    }
});


  document.getElementById("spiffs_sircle").addEventListener('click',function(event) {
    var myLayer = document.getElementById('spiffs_layer');
    if(myLayer.style.display == "block"){
      myLayer.style.display = "none";
    }else{
      myLayer.style.display = "block";
    }
});
  
  document.getElementById("consol_sircle").addEventListener('click',function(event) {
      var textarea = document.getElementById('log');
      if(textarea.style.display == "block"){
        textarea.style.display = "none";
      }else{
        textarea.style.display = "block";
      }
  });

  const ssid_span = document.getElementById('_SSID');
  ssid_span.style.backgroundColor = "#00ff00";
  const ip_span = document.getElementById('_IP');
  ip_span.style.backgroundColor = "#00ff00";

  var mySpan1 = document.getElementById('spiffs_sircle');
  mySpan1.addEventListener('mouseover', () => {
    mySpan1.style.backgroundColor = '#0f0';
  });
  mySpan1.addEventListener('mouseout', () => {
    mySpan1.style.backgroundColor = 'transparent';
});

var mySpan2 = document.getElementById('consol_sircle');
mySpan2.addEventListener('mouseover', () => {
  mySpan2.style.backgroundColor = '#0f0';
});
mySpan2.addEventListener('mouseout', () => {
  mySpan2.style.backgroundColor = 'transparent';
});

}
///////////////////////
document.addEventListener('keydown', function(event) {
 
  // key 's' opens SPIFFS MEMORY popup
  if (event.key == 's') {
    console.log('The "s" key was pressed');
    ShowHide("spiffs_layer");
  }

  // key 'd' opens DATALOG popup
  if (event.key == 'd') {
    console.log('The "d" key was pressed');
    ShowHide("log");
  }
});
#include <Powerbaas.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <uri/UriBraces.h>

const char* ssid = "YOUR-SSID"; // TODO REMOVE!
const char* password = "YOUR-PASSWORD"; // TODO REMOVE!

Powerbaas powerbaas(true);
MeterReading meterReading;
WebServer server(80);
Timer timer1;
ConditionService conditionService;

void setup() {
  Serial.begin(115200);
  delay(3000);
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
  }
  //setupPowerbaas(); // TODO REMOVE!
  setupWebserver();
  setupEndpoints();
  setupConditionMachine();
}

void setupPowerbaas() {
  powerbaas.onMeterReading([](const MeterReading& _meterReading) {
    meterReading = _meterReading;
  });
  powerbaas.setup();
}

void setupWebserver() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Start mDNS
  Serial.println("");
  if (MDNS.begin("powerbaas")) {
    Serial.println("Connect to webserver: http://powerbaas.local");
  }
  Serial.print("Connect to webserver: http://");
  Serial.println(WiFi.localIP());
}

void setupEndpoints() {

  // index
  server.on("/", []() {
    server.send(200, "text/html", index());
  });
  // add switch form
  server.on("/switch/add", []() {
    server.send(200, "text/html", addSwitch());
  });
  // edit switch form
  server.on(UriBraces("/switch/edit/{}"), []() {
    String switchId = server.pathArg(0);
    std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
    ConditionDevice& device = devices[switchId.toInt()];
    server.send(200, "text/html", editSwitch(device));
  });
  // edit switch form
  server.on(UriBraces("/switch/delete/{}"), []() {
    String switchId = server.pathArg(0);
    std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
    ConditionDevice& device = devices[switchId.toInt()];
    server.send(200, "text/html", deleteSwitch(device));
  });
  // pair mode form
  server.on(UriBraces("/switch/pair/{}"), []() {
    String switchId = server.pathArg(0);
    std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
    ConditionDevice& device = devices[switchId.toInt()];
    server.send(200, "text/html", pairSwitch(device));
  });
  // pair mode form
  server.on(UriBraces("/handle/switch/pair/{}"), []() {
    String switchId = server.pathArg(0);
    std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
    ConditionDevice& device = devices[switchId.toInt()];
    server.send(200, "text/html", pairSwitchHandler(device));
    handleSwitchPair(device);
  });


  // handle add switch form
  server.on(UriBraces("/handle/switch/add"), []() {
    handleSwitchAdd(server.arg("name"));
    server.send(200, "text/html", redirect());
  });
  // handle edit switch form
  server.on(UriBraces("/handle/switch/edit"), []() {
    String switchId = server.arg("id");
    String name = server.arg("name");
    std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
    ConditionDevice& device = devices[switchId.toInt()];
    handleSwitchEdit(device, name);
    server.send(200, "text/html", redirect());
  });
  // handle delete switch form
  server.on(UriBraces("/handle/switch/delete/{}"), []() {
    String switchId = server.pathArg(0);
    std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
    ConditionDevice& device = devices[switchId.toInt()];
    handleSwitchDelete(device);
    server.send(200, "text/html", redirect());
  });
  // turn switch on
  server.on(UriBraces("/handle/switch/on/{}"), []() {
    String switchId = server.pathArg(0);
    std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
    ConditionDevice& device = devices[switchId.toInt()];
    handleSwitchOn(device);
    server.send(200, "text/html", redirect());
  });
  // turn switch off
  server.on(UriBraces("/handle/switch/off/{}"), []() {
    String switchId = server.pathArg(0);
    std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
    ConditionDevice& device = devices[switchId.toInt()];
    handleSwitchOff(device);
    server.send(200, "text/html", redirect());
  });

  server.begin();
}

void setupConditionMachine() {

  conditionService.loadConditionDevices();
  // timer1.eachSecond.conditionMachine.run();
}

void loop() {
  powerbaas.receive();
  server.handleClient();
}



/***************************************
 *** Process actions from HTML pages ***
 ***************************************/

void handleSwitchAdd(String name) {
  ConditionDevice device;
  device.id = esp_random() % 10000000;
  strcpy(device.name, name.c_str());
  device.state = DEVICE_OFF;
  device.type = DEVICE_SWITCH;
  std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
  devices[device.id] = device;
  conditionService.storeConditionDevices();
}

void handleSwitchEdit(ConditionDevice& device, String name) {
  strcpy(device.name, name.c_str());
  conditionService.storeConditionDevices();
}

void handleSwitchDelete(ConditionDevice& device) {
  std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
  devices.erase(device.id);
  conditionService.storeConditionDevices();
}

void handleSwitchOn(ConditionDevice& device) {
  device.state = DEVICE_ON;
  NewRemoteTransmitter transmitter(device.id, 12, 232, 3);
  transmitter.sendUnit(0, true);
  // todo: overrule
}

void handleSwitchOff(ConditionDevice& device) {
  device.state = DEVICE_OFF;
  NewRemoteTransmitter transmitter(device.id, 12, 232, 3);
  transmitter.sendUnit(0, false);
  // todo: overrule
}
void handleSwitchPair(ConditionDevice& device) {
  NewRemoteTransmitter transmitter(device.id, 12, 232, 3);
  for(int i = 0; i < 9; i++) {
    transmitter.sendUnit(0, true);
    delay(900);
  }
}



/***************************************
 *** HTML pages and forms **************
 ***************************************/
String index() {

  String body = "<h3>Current power usage</h3>\n";
  body += "<div class=\"card card-on\">" + String(meterReading.powerUsage) + " Watt</div>\n";

  // Switch
  body += "<h3>Smart Switches</h3>\n";
  std::unordered_map<uint8_t, ConditionDevice>& devices = conditionService.getConditionDevices();
  for (auto& deviceElement: devices) {
    ConditionDevice& device = deviceElement.second;
    if(device.state == DEVICE_ON) {
      body += "<a href=\"/switch/edit/" + String(device.id) + "\" class=\"card card-on\">" + String(device.name) + "</a>\n";
    }
    else if(device.state == DEVICE_OFF) {
      body += "<a href=\"/switch/edit/" + String(device.id) + "\" class=\"card card-off\">" + String(device.name) + "</a>\n";
    }
  }
  body += "<a href=\"/switch/add\" class=\"card card-grey\">" + String("+") + "</a>\n";

  return html(body);
}

String addSwitch() {

  String body = "<h3>Add Smart Switch</h3>\n";
  body += "<form action=\"/handle/switch/add\" method=\"GET\">\n";
  body += "<div>Name:</div><div><input type=\"text\" name=\"name\"</div>\n";
  body += "<div><input type=\"submit\" value=\"Save\" /></div>\n";
  body += "</form>\n";

  return html(body);
}

String editSwitch(ConditionDevice& device) {

  String body = "<h3>Edit Smart Switch " + String(device.name) + " (" + String(device.id) + ") </h3>\n";
  body += "<form action=\"/handle/switch/edit\" method=\"GET\">\n";
  body += "<input type=\"hidden\" name=\"id\" value=\"" + String(device.id) + "\">";
  body += "<div>Name:</div><div><input type=\"text\" value=\"" + String(device.name) + "\" name=\"name\"></div>\n";
  body += "<div><input type=\"submit\" /></div>\n";
  body += "</form>";
  body += "<h3>Actions</h3>";
  if(device.state == DEVICE_ON) {
    body += "<a href=\"/handle/switch/off/" + String(device.id) + "\" class=\"card card-on\">Turn Off</a>\n";
  }
  else if(device.state == DEVICE_OFF) {
    body += "<a href=\"/handle/switch/on/" + String(device.id) + "\" class=\"card card-off\">Turn On</a>\n";
  }
  body += "<a href=\"/switch/pair/" + String(device.id) + "\" class=\"card card-action\">Pair</a>\n";
  body += "<a href=\"/switch/delete/" + String(device.id) + "\" class=\"card card-red\">Delete</a>\n";

  return html(body);
}

String deleteSwitch(ConditionDevice& device) {
  String body = "<h3>Delete Smart Switch " + String(device.name) + " </h3>\n";
  body += "<div>Are you sure you want to delete this switch?<br /><br /></div>\n";
  body += "<a href=\"/handle/switch/delete/" + String(device.id) + "\" class=\"card card-red\">Yes!</a>\n";
  body += "<a href=\"/\" class=\"card card-grey\">Cancel</a>\n";

  return html(body);
}

String pairSwitch(ConditionDevice& device) {
  String body = "<h3>Pair Smart Switch " + String(device.name) + " </h3>\n";
  body += "<div>First press <b>Start Pairing</b> and then put switch in socket.<br /><br />The pairing procedure will take about 10 seconds.<br /><br /></div>\n";
  body += "<a href=\"/handle/switch/pair/" + String(device.id) + "\" class=\"card card-action\">Start Pairing</a>\n";

  return html(body);
}

String pairSwitchHandler(ConditionDevice& device) {
  String body = "<h3>Smart Switch " + String(device.name) + " is now pairing</h3>\n";
  body += "<div class=\"card card-action\" id=\"countdown\">10</div>\n";
  body += R"END(
  <script language="javascript">
    var timeleft = 10;
    var downloadTimer = setInterval(function(){
      if(timeleft <= 0){
        clearInterval(downloadTimer);
        document.getElementById("countdown").innerHTML = "Finished";
        window.location = "/";
      } else {
        document.getElementById("countdown").innerHTML = timeleft + "";
      }
      timeleft -= 1;
    }, 1000);
  </script>
  )END";
  return html(body);
}

String redirect() {
  String ptr = R"END(
  <!DOCTYPE html>
  <html>
  <head>
  <script language=javascript>
  function redirect(){
    window.location = "/";
  }
  </script>
  </head>
  <body onload="redirect()">
  </body>
  </html>
  )END";
  return ptr;
}

String html(String body) {
  String ptr = R"END(
  <!DOCTYPE html>
  <html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <!-- <meta http-equiv="refresh" content="2"> -->
  <title>PowerBaas</title>
  <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
    body{margin-top: 20px;} h3 {color: #2d2d2d;margin-bottom: 10px margin-top:50px;}
    .card {display: inline-block;width: 100px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}
    .card-small{width:40px; display:inline-block;}
    .card-on {background-color: #38b54a;}
    .card-off {background-color: #2d2d2d;}
    .card-action {background-color: #ece50a;}
    .card-grey {background-color: #dddddd;}
    .card-red {background-color: #CC0000; }
    p {font-size: 14px;color: #2d2d2d;margin-bottom: 10px;}
    input { padding:10px; border-radius:10px; border:1px solid #cccccc; }
    input[type=submit] { margin-top:25px; }
  </style>
  </head>
  <body>
  <a href="/">
  <img style='border:none; width:80%; margin-bottom:25px;' src="https://t.eu1.jwwb.nl/W1292300/7Z67ppdpu6A8hHiS3s7fsxO1-ys=/658x0/filters:quality(70)/f.eu1.jwwb.nl%2Fpublic%2Fs%2Fq%2Fx%2Ftemp-ubafshmstbomdhzsimxh%2Fum7ocq%2Fimage-1.png" />
  </a>
  )END";

  ptr += body;

  ptr += "</body>\n";
  ptr += "</html>\n";

  return ptr;
}
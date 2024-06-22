#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

#define FORCE_SENSOR_PIN 36 // ESP32 pin GPIO36 (ADC0): the FSR and 10K pulldown are connected to A0

//const char* ssid = "TNCAP204017";
//const char* password = "7C1A66CF5F";
const char* ssid = "IoTRouter1";
const char* password = "1234567890";

WebServer server(80);  // Object of WebServer(HTTP port, 80 is default)
Servo myServo;

int analogReading = 0;
String description = "no pressure";
int savedNumber = 0;  // Variable to store the submitted number

void setup() {
  Serial.begin(115200);
  myServo.attach(18);

  WiFi.mode(WIFI_STA);
  ConnectToWifi(ssid, password);  // Function to connect to WiFi

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/save", handleSave);
  server.on("/submit", handleSubmit);
  server.on("/cancel", handleCancel);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  analogReading = analogRead(FORCE_SENSOR_PIN);

  if (analogReading < 10) {
    description = "no pressure";
  } else if (analogReading < 1000) {
    description = "light touch";
  } else if (analogReading < 2000) {
    description = "light squeeze";
  } else if (analogReading < 3000) {
    description = "medium squeeze";
  } else {
    description = "big squeeze";
  }

  server.handleClient();
  delay(1000);
}

void handleRoot() {
  String htmlPage = "<html><head><style>";
  htmlPage += "body { display: flex; flex-direction: column; justify-content: center; align-items: center; height: 100vh; font-family: Arial, sans-serif; }";
  htmlPage += ".container { text-align: center; }";
  htmlPage += "input, button { font-size: 16px; margin: 10px; padding: 10px 20px; }";
  htmlPage += "</style></head><body>";
  htmlPage += "<div class='container'>";
  htmlPage += "<h1>ESP32 Web Server</h1><br><br>";
  htmlPage += "<label id='pressureLabel'>Pressure: 0</label><br><br>";
  htmlPage += "<label id='descriptionLabel'>Description: no pressure</label><br><br>";
  htmlPage += "<button onclick=\"saveData()\">Save</button><br><br>";
  htmlPage += "<h1>Motor Controller</h1>";
  htmlPage += "<input type='text' id='numberInput'><br><br>";
  htmlPage += "<button onclick=\"submitNumber()\">Submit</button>&nbsp;&nbsp;";
  htmlPage += "<button onclick=\"cancelOperation()\">Cancel</button>";
  htmlPage += "</div>";
  htmlPage += "<script>";
  htmlPage += "function updateData() {";
  htmlPage += "  var xhttp = new XMLHttpRequest();";
  htmlPage += "  xhttp.onreadystatechange = function() {";
  htmlPage += "    if (this.readyState == 4 && this.status == 200) {";
  htmlPage += "      var response = JSON.parse(this.responseText);";
  htmlPage += "      document.getElementById('pressureLabel').innerHTML = 'Pressure: ' + response.pressure;";
  htmlPage += "      document.getElementById('descriptionLabel').innerHTML = 'Description: ' + response.description;";
  htmlPage += "    }";
  htmlPage += "  };";
  htmlPage += "  xhttp.open('GET', '/data', true);";
  htmlPage += "  xhttp.send();";
  htmlPage += "}";
  htmlPage += "setInterval(updateData, 1000);";  // Automatically refresh data every 1 second
  htmlPage += "function saveData() {";
  htmlPage += "  var xhttp = new XMLHttpRequest();";
  htmlPage += "  xhttp.open('GET', '/save', true);";
  htmlPage += "  xhttp.send();";
  htmlPage += "}";
  htmlPage += "function submitNumber() {";
  htmlPage += "  var xhttp = new XMLHttpRequest();";
  htmlPage += "  var number = document.getElementById('numberInput').value;";
  htmlPage += "  xhttp.open('GET', '/submit?number=' + number, true);";
  htmlPage += "  xhttp.send();";
  htmlPage += "}";
  htmlPage += "function cancelOperation() {";
  htmlPage += "  document.getElementById('numberInput').value = '';";
  htmlPage += "  var xhttp = new XMLHttpRequest();";
  htmlPage += "  xhttp.open('GET', '/cancel', true);";
  htmlPage += "  xhttp.send();";
  htmlPage += "}";
  htmlPage += "</script>";
  htmlPage += "</body></html>";
  server.send(200, "text/html", htmlPage);
}

void handleData() {
  String jsonResponse = "{\"pressure\":" + String(analogReading) + ",\"description\":\"" + description + "\"}";
  server.send(200, "application/json", jsonResponse);
}

void handleSave() {
  String serverName = "http://192.168.0.103/IOT_FP/save_data.php";
  WiFiClient client;
  HTTPClient http;  // Declare an object of class HTTPClient
  String url = serverName + "?pressure=" + String(analogReading) + "&description=" + description;
  Serial.println("Sending request to: " + url);  // Debugging output
  http.begin(client, url);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.println("Error saving data.");
  }
  http.end();
  server.send(200, "text/plain", "Data save request sent.");
}

void handleSubmit() {
  if (server.hasArg("number")) {
    savedNumber = server.arg("number").toInt();
    Serial.print("Number saved: ");
    Serial.println(savedNumber);
    server.send(200, "text/plain", "Number submitted and saved.");
    myServo.write(savedNumber);
  } else {
    server.send(400, "text/plain", "Number not provided.");
  }
}

void handleCancel() {
  Serial.println("Operation cancelled.");
  server.send(200, "text/plain", "Operation cancelled.");
}

void ConnectToWifi(const char* ssid, const char* password) {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

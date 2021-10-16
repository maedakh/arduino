#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#define PULSE_PIN D2 //gpio4
#define LED_PIN D7 //gpio13


const char * ssid = "Server";
const char * password = "lannister";

String serverName = "http://192.168.0.100/index.php";
unsigned long lastTime = 0;
//unsigned long timerDelay = 1000;
unsigned long timerDelay = 1800000;

volatile long pulseCount = 0;
float calibrationFactor = 4.5;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
float totalLitres;
unsigned long oldTime;

void ICACHE_RAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  oldTime = 0;

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(PULSE_PIN, INPUT);
  attachInterrupt(PULSE_PIN, pulseCounter, FALLING);
}

void loop() {
  if ((millis() - oldTime) > 1000) // Only process counters once per second
  {
    detachInterrupt(PULSE_PIN);
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    oldTime = millis();
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;
    totalLitres = totalMilliLitres * 0.001;
    unsigned int frac;
    Serial.print("flowrate: ");
    Serial.print(int(flowRate)); // Print the integer part of the variable
    Serial.print("."); // Print the decimal point
    frac = (flowRate - int(flowRate)) * 10;
    Serial.print(frac, DEC); // Print the fractional part of the variable
    Serial.print("L/min");
    Serial.print("  Current Liquid Flowing: "); // Output separator
    Serial.print(flowMilliLitres);
    Serial.print("mL/Sec");
    Serial.print("  Output Liquid Quantity: "); // Output separator
    Serial.print(totalLitres);
    Serial.println("L");

    pulseCount = 0;

    attachInterrupt(PULSE_PIN, pulseCounter, FALLING);

  }

  // Send an HTTP POST request depending on timerDelay
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      String serverPath = serverName + "?action=send&sensor=4&value=" + totalLitres/1.5;

      // Your Domain name with URL path or IP address with path
      http.begin(client, serverPath.c_str());

      // Send HTTP GET request
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        totalMilliLitres = 0;
        totalLitres = 0.0;
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }

      // Free resources
      http.end();
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

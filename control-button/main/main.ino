#include "WiFiS3.h"
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include "secrets.h"

// Hardware configuration
#define DHTPIN 2
#define DHTTYPE DHT11
const int ledPin = 3;

// Objects
DHT dht(DHTPIN, DHTTYPE);
WiFiServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Global state
String lampStatus = "OFF";
unsigned long lastSensorUpdate = 0;

void setup() {
  Serial.begin(115200);
  delay(2000); 
  
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");

  Serial.println("Starting...");
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.print("Connecting to WiFi: ");
  Serial.println(SECRET_SSID);
  lcd.setCursor(0, 1);
  lcd.print("WiFi: Connect...");
  
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  server.begin();
  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    int ip_retry = 0;
    while (WiFi.localIP().toString() == "0.0.0.0" && ip_retry < 10) {
      delay(500);
      ip_retry++;
    }
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi FAILED!");
    lcd.print("WiFi FAILED!");
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Periodic sensor and display update (every 2 seconds)
  if (currentMillis - lastSensorUpdate >= 2000) {
    lastSensorUpdate = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Line 1: Clear and show IP
    lcd.setCursor(0, 0);
    lcd.print(WiFi.localIP());
    lcd.print("        "); // Clear leftover chars

    // Line 2: Show Temp, Hum and Lamp
    lcd.setCursor(0, 1);
    if (isnan(h) || isnan(t)) {
      lcd.print("Err ");
    } else {
      lcd.print((int)t); lcd.print("C ");
      lcd.print((int)h); lcd.print("% ");
    }

    lcd.setCursor(10, 1);
    lcd.print("L:");
    lcd.print(lampStatus);
    lcd.print("  ");
  }

  WiFiClient client = server.available();
  if (client) {
    String requestLine = "";
    while (client.connected() && client.available()) {
      char c = client.read();
      if (c == '\n') break;
      requestLine += c;
    }

    if (requestLine.indexOf("POST /api/lamp") >= 0) {
      // Clear headers until body
      while (client.available()) {
        char c = client.peek();
        if (c == '{') break;
        client.read();
      }

      StaticJsonDocument<128> doc;
      DeserializationError error = deserializeJson(doc, client);

      if (!error) {
        const char* status = doc["status"];
        if (strcmp(status, "on") == 0) {
          digitalWrite(ledPin, HIGH);
          lampStatus = "ON ";
        } else if (strcmp(status, "off") == 0) {
          digitalWrite(ledPin, LOW);
          lampStatus = "OFF";
        }
        sendResponse(client, 200, "OK");
      } else {
        sendResponse(client, 400, "Bad Request");
      }
    }
    client.stop();
  }
}

void sendResponse(WiFiClient& client, int code, String msg) {
  client.print("HTTP/1.1 "); client.print(code); client.print(" "); client.println(msg);
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.print("{\"result\":\""); client.print(msg); client.println("\"}");
}

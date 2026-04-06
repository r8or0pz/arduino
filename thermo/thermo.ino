#include "WiFiS3.h"
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "DHT.h"
#include "secrets.h"

#define DHTPIN 2
#define DHTTYPE DHT11
const int ledPin = 3; // Сюда подключаем контакт "S"

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiServer server(80);

String lampStatus = "OFF";
unsigned long lastSensorUpdate = 0;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  dht.begin();
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("WiFi Connect...");

  WiFi.begin(SECRET_SSID, SECRET_PASS);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(1000);
    retry++;
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    server.begin();
    lcd.clear();
  } else {
    lcd.clear();
    lcd.print("WiFi Error!");
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastSensorUpdate >= 2000) {
    lastSensorUpdate = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // ВЫВОД IP: Убрали двоеточие, чтобы влезли все цифры
    lcd.setCursor(0, 0);
    lcd.print(WiFi.localIP());
    lcd.print("    ");

    // ВЫВОД ДАТЧИКОВ И ЛАМПЫ
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
    processRequest(client);
  }
}

void processRequest(WiFiClient& client) {
  String requestLine = "";
  while (client.connected() && client.available()) {
    char c = client.read();
    if (c == '\n') break;
    requestLine += c;
  }

  if (requestLine.indexOf("POST /api/lamp") >= 0) {
    while (client.available()) {
      String line = client.readStringUntil('\r');
      if (line == "\n") break;
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
  delay(10);
  client.stop();
}

void sendResponse(WiFiClient& client, int code, String msg) {
  client.print("HTTP/1.1 "); client.print(code); client.print(" "); client.println(msg);
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.print("{\"result\":\""); client.print(msg); client.println("\"}");
}
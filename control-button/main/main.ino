#include "WiFiS3.h"
#include <ArduinoJson.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WiFiServer server(80);
const int ledPin = 2; // Пин для внешнего светодиода

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(1000);

  server.begin();
  Serial.println(WiFi.localIP());
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String requestLine = "";
    while (client.connected() && client.available()) {
      char c = client.read();
      if (c == '\n') break;
      requestLine += c;
    }

    // Проверка эндпоинта и метода
    if (requestLine.indexOf("POST /api/lamp") >= 0) {
      // Пропускаем заголовки до тела JSON
      while (client.available()) {
        String line = client.readStringUntil('\r');
        if (line == "\n") break;
      }

      StaticJsonDocument<128> doc;
      DeserializationError error = deserializeJson(doc, client);

      if (!error) {
        const char* status = doc["status"];
        if (strcmp(status, "on") == 0) digitalWrite(ledPin, HIGH);
        else if (strcmp(status, "off") == 0) digitalWrite(ledPin, LOW);

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

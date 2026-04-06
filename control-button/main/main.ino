#include "WiFiS3.h"
#include <ArduinoJson.h>
#include "secrets.h"

WiFiServer server(80);
const int ledPin = 2; // Пин для внешнего светодиода

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  WiFi.begin(SECRET_SSID, SECRET_PASS);

  const unsigned long wifiConnectTimeoutMs = 15000;
  unsigned long wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < wifiConnectTimeoutMs) {
    delay(1000);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    while (true) {
      digitalWrite(ledPin, HIGH);
      delay(250);
      digitalWrite(ledPin, LOW);
      delay(250);
    }
  }
  server.begin();
  Serial.println(WiFi.localIP());
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String requestLine = "";
String readHttpLine(WiFiClient& client) {
  String line = "";
  unsigned long start = millis();
  while (client.connected() && (millis() - start) < 2000) {
    while (client.available()) {
      char c = client.read();
      if (c == '\r') continue;
      if (c == '\n') return line;
      line += c;
    }
  }
  return line;
}

bool readRequestBody(WiFiClient& client, size_t contentLength, String& body) {
  body = "";
  body.reserve(contentLength);

  unsigned long start = millis();
  while (client.connected() && body.length() < contentLength && (millis() - start) < 2000) {
    while (client.available() && body.length() < contentLength) {
      body += (char)client.read();
      start = millis();
    }
  }

  return body.length() == contentLength;
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String requestLine = readHttpLine(client);

    int contentLength = -1;
    while (client.connected()) {
      String line = readHttpLine(client);
      if (line.length() == 0) break;

      if (line.startsWith("Content-Length:")) {
        String value = line.substring(String("Content-Length:").length());
        value.trim();
        contentLength = value.toInt();
      }
    }

    // Проверка эндпоинта и метода
    if (requestLine.indexOf("POST /api/lamp") >= 0) {
      if (contentLength <= 0) {
        sendResponse(client, 400, "Bad Request");
      } else {
        String body;
        if (!readRequestBody(client, (size_t)contentLength, body)) {
          sendResponse(client, 400, "Bad Request");
        } else {
          StaticJsonDocument<128> doc;
          DeserializationError error = deserializeJson(doc, body);

          if (!error) {
            const char* status = doc["status"];
            if (status != NULL) {
              if (strcmp(status, "on") == 0) digitalWrite(ledPin, HIGH);
              else if (strcmp(status, "off") == 0) digitalWrite(ledPin, LOW);
              sendResponse(client, 200, "OK");
            } else {
              sendResponse(client, 400, "Bad Request");
            }
          } else {
            sendResponse(client, 400, "Bad Request");
          }
        }
      }
    } else if (requestLine.indexOf("/api/lamp") >= 0) {
      sendResponse(client, 405, "Method Not Allowed");
    } else {
      sendResponse(client, 404, "Not Found");
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

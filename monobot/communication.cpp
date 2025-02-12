#include "HardwareSerial.h"
#include "communication.h"
#include <Arduino.h>

Communication::Communication()
  : _server(80) 
{

}

void Communication::setupWIFI() {
  if (!Serial.available()) {
    delay(1000);
  }

  // Connect to WiFi
  IPAddress ipadrr;
  WiFi.begin(_ssid, _pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  ipadrr = WiFi.localIP();
  Serial.print("ESP8266 IP address: ");
  Serial.println(ipadrr);

  _server.begin();

  // Start the MDNS responder
  if (!MDNS.begin("monobot")) { // Use your preferred hostname
      Serial.println("Error setting up MDNS responder!");
  } else {
      Serial.println("MDNS responder started. Access via http://monobot.local/");
  }

  ipadrr = WiFi.localIP();
  Serial.print("Arduino IP address: ");
  Serial.println(ipadrr);
}

void Communication::MDNSUpdate() {
  // Check for incoming client connections
  if (!_client || !_client.connected()) {
    // If no client or client disconnected, check for new clients
    WiFiClient newClient = _server.available();
    if (newClient) {
      _client = newClient;
    }
  }
  MDNS.update();
}

char* Communication::recieveData() {
    static char buffer[400]; // Statischer Puffer für empfangene Daten
    memset(buffer, 0, sizeof(buffer)); // Puffer leeren
    
    if (_client && _client.connected() && _client.available()) {
        //Serial.println("Waiting for data")
        size_t length = _client.readBytesUntil(MESSAGE_TERMINATOR, buffer, sizeof(buffer) - 1);
        buffer[length] = '\0'; // Nullterminierung für sicheren String
        Serial.println(buffer);
        return buffer;
    }
    
    return nullptr; // Kein neuer Datenempfang
}

char Communication::recieveDataStream() {
  return _client.read();
}

void Communication::sendLog(LogMsg logMsg, const Config* config) {
  String serialMsg;
  if (static_cast<int>(logMsg.level) < static_cast<int>(config->LOG_LEVEL)) {
    serialMsg = "No Log sent:" + String((int)logMsg.level) + " < " + String((int) config->LOG_LEVEL);
    Serial.println(serialMsg);
    return;
  }
  serialMsg = "Sending Log: " + String(static_cast<int>(logMsg.level)) + " " + String(static_cast<int>(config->LOG_LEVEL)) + ": " + logMsg.message; 
  Serial.println(serialMsg);

  _client.println(logMsg.message);
}

bool Communication::isClientAvailable() {
  return _client.available();
}

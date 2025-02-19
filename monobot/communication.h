#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "config.h"

class Communication {
private:
  // WiFi Settings
  const char* _ssid = "Mi Note 10";
  const char* _pass = "brockmann";

  const char MESSAGE_TERMINATOR = 0x1E;

  // Create a WiFi server on port 80
  WiFiServer _server;
  WiFiClient _client;

public:
  Communication();
  void setupWIFI();
  char* recieveData();
  char recieveDataStream(); //Deprecated
  int receiveState();
  void sendLog(LogMsg logMsg, const Config* config);
  void MDNSUpdate();
  bool isClientAvailable();
};

#endif // COMMUNICATION_H

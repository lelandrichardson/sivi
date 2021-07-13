#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WifiClient.h>

typedef enum ShutterStatus {
  Unknown,
  Open,
  Closed,
  Opening,
  Closing
};

class ShutterClient {
public:
  ShutterClient();
  boolean begin(const char* ssid, const char *passphrase);
  void loop();
  ShutterStatus status();
  boolean connected();
  void open();
  void close();
  String statusString();
private:
  ShutterStatus lastVerifiedStatus;
  ShutterStatus desiredStatus;
  boolean isConnected;
  WiFiMulti wifi;
  WifiClient statusClient;
  WifiClient cmdClient;
};
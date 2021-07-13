#include "ShutterClient.h"

ShutterClient::ShutterClient() {
}

boolean ShutterClient::begin(const char* ssid, const char *passphrase) {
  wifi.addAP("SSID", "passpasspass");
  return true;
}

ShutterStatus ShutterClient::status() {
  return Open;
}

boolean ShutterClient::connected() {
  return wifi.run() == WL_CONNECTED;
  return true;
}

void ShutterClient::loop() {

  const uint16_t port = 1337;
  const char * host = "192.168.1.10"; // ip or dns

  WiFiClient client;

  while (wifi.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }


  if (!client.connect(host, port)) {
      Serial.println("Connection failed.");
      Serial.println("Waiting 5 seconds before retrying...");
      delay(5000);
      return;
  }

  int maxloops = 0;

  //wait for the server's reply to become available
  while (!client.available() && maxloops < 1000)
  {
    maxloops++;
    delay(1); //delay 1 msec
  }
  if (client.available() > 0)
  {
    //read back one line from the server
    String line = client.readStringUntil('\r');
    Serial.println(line);
  }
  else
  {
    Serial.println("client.available() timed out ");
  }

    Serial.println("Closing connection.");
    client.stop();

    Serial.println("Waiting 5 seconds before restarting...");
    delay(5000);

}

void ShutterClient::open() {

}

void ShutterClient::close() {
  
}

String ShutterClient::statusString() {
  switch (status()) {
    case Open: return F("OPEN");
    case Closed: return F("CLOSED");
    case Opening:return F("OPENING");
    case Closing: return F("CLOSING");
    default: return F("UNKNOWN");
  }
}
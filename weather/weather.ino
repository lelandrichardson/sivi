#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "src/Constants.h"
#include "wifi_credentials.h"
#include "WeatherStation.h"

#ifndef WIFI_CREDENTIALS
#define WIFI_CREDENTIALS
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PWD "your-wifi-pwd"
#endif

#define PIN_RG11 15


// SDA = 4 (blue)
// SCL = 5 (yellow)

WeatherStation weather = WeatherStation(
  &Wire,
  PIN_RG11
);

// ASCOM Observing Conditions
// ==========================
// Temperature
// SkyTemperature
// SkyQuality
// SkyBrightness
// RainRate
// Pressure
// Humidity
// DewPoint
// CloudCover
// Connected
//
// WindSpeed
// WindGust
// WindDirection
// StarFWHM
// AveragePeriod



// TODO:
// - wifi tcp/http connection, response?
// - serial ascom interface?
// - rg11
// - wind sensor
// - http send back bitmap for wind data

unsigned long serialLastPrint = 0;

void loopSerial() {
  if (micros() - serialLastPrint > 5 * MICROS_PER_SEC) {
    serialLastPrint = micros();
    Serial.print("Temp: ");
    Serial.print(weather.ambientTemperature());
    Serial.println(" C");

    Serial.print("Pressure: ");
    Serial.print(weather.pressure());
    Serial.println(" kPa");

    Serial.print("Humidity: ");
    Serial.print(weather.humidity());
    Serial.println(" %");

    Serial.print("Altitude: ");
    Serial.print(weather.altitude());
    Serial.println(" m");


    Serial.print("Luminosity: ");
    Serial.print(weather.luminosity());
    Serial.println(" lux");

    Serial.print("Clouds: ");
    Serial.print(weather.percentCloudCover());
    Serial.println(" %");
    Serial.println();
  }
}

// Server

const char* ssid = WIFI_SSID;
const char* password = WIFI_PWD;
ESP8266WebServer server(80);

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void handleRoot() {
  server.send(200, "text/plain", weather.boltwoodData());
}

void handleAscomRequest() {
  String command = server.arg("cmd");
  if (command.equals(""))

  server.send(200, "text/plain", "hello from esp8266!");
}

void setupWiFi() {
  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/ascom", handleAscomRequest);
  server.onNotFound(handleNotFound);

  server.begin();

}

void setup() {
  Serial.begin(115200);
  while (!Serial); // Waiting for Serial Monitor
  Wire.begin();
  if (!weather.begin()) {
    Serial.println(weather.error);
    while (true) delay(10);
  }
  setupWiFi();
  delay(1000);
}

void loop() {
  weather.loop();
  server.handleClient();
  MDNS.update();
  loopSerial();
}

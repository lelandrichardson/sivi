#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "src/Constants.h"
#include "wifi_credentials.h"
#include "WeatherStation.h"
// #include "src/WebCommander.h"
// #include "src/SerialCommander.h"

#ifndef WIFI_CREDENTIALS
#define WIFI_CREDENTIALS
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PWD "your-wifi-pwd"
#endif

#define PIN_RG11 15
#define PIN_MDWIND A0


// SDA = 4 (blue)
// SCL = 5 (yellow)

WeatherStation weather(
  &Wire,
  PIN_RG11,
  PIN_MDWIND
);

// WebCommander web(
//   "weather",
//   ssid, 
//   password
// );
// SerialCommander ascom = SerialCommander();

// void setupSerialAscom() {
//   ascom.begin();
//   ascom.on(F("NAME"), []() {
//     ascom.success("weather");
//   });

//   ascom.on(F("AveragePeriod"), []() {
//     // Time period (hours) over which to average sensor readings
//     ascom.success(0.0); // instantaneous readings
//   });
//   ascom.on(F("CloudCover"), []() {
//     // percentage of the sky covered by cloud
//     ascom.success(weather.percentCloudCover());
//   });
//   ascom.on(F("DewPoint"), []() {
//     // Atmospheric dew point reported in °C.
//     ascom.success(weather.dewPoint());
//   });
//   ascom.on(F("Humidity"), []() {
//     // Atmospheric humidity (%)
//     ascom.success(weather.humidity());
//   });
//   ascom.on(F("Pressure"), []() {
//     // Atmospheric presure at the observatory (hPa)
//     ascom.success(weather.pressure());
//   });
//   ascom.on(F("RainRate"), []() {
//     // Rain rate (mm / hour)
//     // This property can be interpreted as 0.0 = Dry any positive nonzero value = wet.
//     // Rainfall intensity is classified according to the rate of precipitation:
//     // Light rain —  0-2.5
//     // Moderate rain — 2.5-10
//     // Heavy rain — 10-50
//     // Violent rain — 50+

//     // TODO: consider using the rg11 in something other than the simple "is raining" mode
//     if (weather.isRaining()) {
//       ascom.success(3);
//     } else {
//       ascom.success(0)
//     }
//   });
//   ascom.on(F("SkyBrightness"), []() {
//     // Sky brightness (Lux)
//     // 0-0.27 Moonless, overcast night sky (starlight) 
//     // 0.27–1.0 Full moon on a clear night 
//     // 3.4 Dark limit of civil twilight under a clear sky 
//     // 50-500 indoor lighting
//     // 1000 Overcast day; typical TV studio lighting 
//     // 10000+ sunlight 
    
//     // TODO: need to calibrate this value
//     ascom.success(weather.luminosity());
//   });
//   ascom.on(F("SkyTemperature"), []() {
//     // Sky temperature in °C
//     ascom.success(weather.skyTemperature());
//   });
//   ascom.on(F("Temperature"), []() {
//     // Sky temperature in °C
//     ascom.success(weather.ambientTemperature());
//   });
//   ascom.on(F("WindGust"), []() {
//     // Wind gust (m/s) Peak 3 second wind speed over the last 2 minutes
//     ascom.success(weather.windGust());
//   });
//   ascom.on(F("WindSpeed"), []() {
//     // Wind speed (m/s)
//     ascom.success(weather.windSpeed());
//   });
// }

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

    Serial.print("Raining: ");
    Serial.println(weather.isRaining());

    Serial.print("Wind Speed: ");
    Serial.println(weather.windSpeed());

    Serial.print("Wind Gust: ");
    Serial.println(weather.windGust());

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

  if (MDNS.begin("weather")) {
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
  server.on("boltwood", []() {
    server.send(200, "text/plain", weather.boltwoodData());
  });
  setupWiFi();
  delay(1000);
}

void loop() {
  weather.loop();
  server.handleClient();
  MDNS.update();
  loopSerial();
}

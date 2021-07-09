#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_TSL2591.h"
#include <Adafruit_BMP280.h>
#include "Adafruit_SHT31.h"
#include "Adafruit_AMG88xx.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "wifi_credentials.h"

#ifndef WIFI_CREDENTIALS
#define WIFI_CREDENTIALS
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PWD "your-wifi-pwd"
#endif


#define MICROS_PER_MS 1000
#define MICROS_PER_SEC 1000000
#define MILLIS_PER_SEC 1000
#define DEBOUNCE_MICROS 300 * MICROS_PER_MS
#define SLEEP_ENABLED true


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

/* TSL2591 Digital Light Sensor */
/* Dynamic Range: 600M:1 */
/* Maximum Lux: 88K */
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)
uint16_t luminosity = 0;

void setupLightSensor() {
  if (!tsl.begin(&Wire, 0x29)) {
    Serial.println("No TSL2591 sensor found...");
    // TODO: error handling
    while (1) delay(10);
  }
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  //tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain
  
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)
}

void loopLightSensor() {
  // Simple data read example. Just read the infrared, fullspecrtrum diode 
  // or 'visible' (difference between the two) channels.
  // This can take 100-600 milliseconds! Uncomment whichever of the following you want to read
  luminosity = tsl.getLuminosity(TSL2591_VISIBLE);
  //luminosity = tsl.getLuminosity(TSL2591_FULLSPECTRUM);
  //luminosity = tsl.getLuminosity(TSL2591_INFRARED);
}


















/* BMP280 Barometric Pressure & Altitude Sensor */
Adafruit_BMP280 bmp;

// temperature, in celsius
float bmpTemperature = 0.0;

// barometric pressure, in Pascals
float bmpPressure = 0.0;

// altitude of sensor, in meters
float bmpAltitude = 0.0;

void setupBMP() {
  bmp = Adafruit_BMP280(&Wire);
  if (!bmp.begin()) {
    Serial.println("No BMP280 sensor found...");
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
    Adafruit_BMP280::STANDBY_MS_500   /* Standby time. */
  );
}

void loopBMP() {
    bmpTemperature = bmp.readTemperature();
    bmpPressure = bmp.readPressure();
    bmpAltitude = bmp.readAltitude(1013.25); /* Adjusted to local forecast! */
}




























/* SHT31-D Temperature & Humidity Sensor */
Adafruit_SHT31 sht31 = Adafruit_SHT31(&Wire);

// Temperature, in Celsius
float shtTemperature = 0.0;

// Humidity, in percent
float shtHumidity = 0.0;

void setupTempSensor() {
  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("No SHT31 sensor found...");
    while (1) delay(10);
  }
  sht31.heater(false);
}

void loopTempSensor() {
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (!isnan(t)) {  // check if 'is not a number'
    shtTemperature = t;
  }
  
  if (!isnan(h)) {  // check if 'is not a number'
    shtHumidity = h;
  }
}












/* AMG8833 IR Thermal Camera */
Adafruit_AMG88xx amg;
float amgPixels[AMG88xx_PIXEL_ARRAY_SIZE]; // buffer for full frame of temperatures

void setupCloudSensor() {
  if (!amg.begin()) {
    Serial.println("No AMG8833 sensor found...");
    while (1) delay(10);
  }
}

void loopCloudSensor() {
  amg.readPixels(amgPixels);
}







void loopSerial() {
  Serial.print("Temp: ");
  Serial.print(shtTemperature);
  Serial.println(" C");

  Serial.print("Pressure: ");
  Serial.print(bmpPressure);
  Serial.println(" kPa");

  Serial.print("Humidity: ");
  Serial.print(shtHumidity);
  Serial.println(" %");

  Serial.print("Altitude: ");
  Serial.print(bmpAltitude);
  Serial.println(" m");

  Serial.print("Clouds: ");
  Serial.print(123);
  Serial.println(" %");

  Serial.print("[");
  for(int i=1; i<=AMG88xx_PIXEL_ARRAY_SIZE; i++){
    Serial.print(amgPixels[i-1]);
    Serial.print(", ");
    if( i%8 == 0 ) Serial.println();
  }
  Serial.println("]");
  Serial.println();
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
  server.send(200, "text/plain", "hello from esp8266!");
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

void loopWiFi() {
  server.handleClient();
  MDNS.update();
}









void setup() {
  Serial.begin(115200);
  while (!Serial); // Waiting for Serial Monitor
  Wire.begin();
  delay(1000);
  setupLightSensor();
  setupBMP();
  setupTempSensor();
  setupCloudSensor();
  setupWiFi();
  delay(1000);
}

void loop() {
  // slow read!
  loopLightSensor();
  // quick read
  loopBMP();
  // quick read
  loopTempSensor();
  // slow read!
  loopCloudSensor();
  // handle every time
  loopSerial();
  // handle every time
  loopWiFi();
  // TODO: get rid of this delay, use timers for everything
  delay(5000);
}


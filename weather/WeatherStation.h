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
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "MDWindSensor.h"

#define GUST_BUCKETS 4

class WeatherStation {
public:
  WeatherStation(
    TwoWire * wireInst,
    uint8_t rg11Pin,
    uint8_t mdWindPin
  );
  boolean begin();
  void loop();

  float percentCloudCover();
  float ambientTemperature();
  float skyTemperature();
  float dewPoint();
  uint16_t luminosity();
  boolean isRaining();
  float humidity();
  float pressure();
  float altitude();
  float windSpeed();
  float windGust();
  String boltwoodData();
  String error = "";

private:
  uint8_t pin_rg11;
  uint8_t pin_wind;
  boolean rg11Value;
  TwoWire *wire;
  WiFiUDP ntpUDP;
  NTPClient timeClient;

  Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
  uint16_t tslLuminosity = 0;
  unsigned long tslLastRead = 0;

  Adafruit_BMP280 bmp;
  float bmpTemperature = 0.0; // temperature, in celsius
  float bmpPressure = 0.0; // barometric pressure, in Pascals
  float bmpAltitude = 0.0; // altitude of sensor, in meters
  unsigned long bmpLastRead = 0;

  Adafruit_SHT31 sht31 = Adafruit_SHT31(&Wire);
  float shtTemperature = 0.0;
  float shtHumidity = 0.0;
  unsigned long shtLastRead = 0;

  Adafruit_AMG88xx amg;
  float amgPixels[AMG88xx_PIXEL_ARRAY_SIZE]; // buffer for full frame of temperatures
  unsigned long amgLastRead = 0;
  // TODO: make configurable
  float clearAmbientSkyDelta = 15; // initialise delta t for clear sky
  // TODO: make configurable
  float cloudyAmbientSkyDelta = 10; // initialise delta t for cloudy sky

  MDWindSensor windSensor;
  unsigned long windLastBucket = 0;
  // 4 buckets for the max speed recorded in 30 seconds (2 minutes total)
  float windGusts[GUST_BUCKETS] = {0.0, 0.0, 0.0, 0.0};

};
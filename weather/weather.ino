#include <Arduino.h>
#include <Wire.h>
#include "wifi_credentials.h"
#include "src/Constants.h"
#include "src/AscomApi.h"
#include "WeatherStation.h"

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

AscomApi ascom = AscomApi();

const char *ssid = WIFI_SSID;
const char *pwd = WIFI_PWD;

void setupAscom() {
  ascom.begin("weather", ssid, pwd);

  // ascom.propertyString(F("NAME"), []() {
  //   return "weather";
  // });
  ascom.propertyFloat(F("AveragePeriod"), []() {
    // Time period (hours) over which to average sensor readings
    return 0.0;
  });
  ascom.propertyFloat(F("CloudCover"), []() {
    // percentage of the sky covered by cloud
    return weather.percentCloudCover();
  });
  ascom.propertyFloat(F("DewPoint"), []() {
    // Atmospheric dew point reported in °C.
    return weather.dewPoint();
  });
  ascom.propertyFloat(F("Humidity"), []() {
    // Atmospheric humidity (%)
    return weather.humidity();
  });
  ascom.propertyFloat(F("Pressure"), []() {
    // Atmospheric presure at the observatory (hPa)
    return weather.pressure();
  });
  ascom.propertyFloat(F("RainRate"), []() {
    // Rain rate (mm / hour)
    // This property can be interpreted as 0.0 = Dry any positive nonzero value = wet.
    // Rainfall intensity is classified according to the rate of precipitation:
    // Light rain —  0-2.5
    // Moderate rain — 2.5-10
    // Heavy rain — 10-50
    // Violent rain — 50+

    // TODO: consider using the rg11 in something other than the simple "is raining" mode
    if (weather.isRaining()) {
      return 3.0;
    } else {
      return 0.0;
    }
  });
  ascom.propertyFloat(F("SkyBrightness"), []() {
    // Sky brightness (Lux)
    // 0-0.27 Moonless, overcast night sky (starlight) 
    // 0.27–1.0 Full moon on a clear night 
    // 3.4 Dark limit of civil twilight under a clear sky 
    // 50-500 indoor lighting
    // 1000 Overcast day; typical TV studio lighting 
    // 10000+ sunlight 
    
    // TODO: need to calibrate this value
    return weather.luminosity();
  });
  ascom.propertyFloat(F("SkyTemperature"), []() {
    // Sky temperature in °C
    return weather.skyTemperature();
  });
  ascom.propertyFloat(F("Temperature"), []() {
    // Sky temperature in °C
    return weather.ambientTemperature();
  });
  ascom.propertyFloat(F("WindGust"), []() {
    // Wind gust (m/s) Peak 3 second wind speed over the last 2 minutes
    return weather.windGust();
  });
  ascom.propertyFloat(F("WindSpeed"), []() {
    // Wind speed (m/s)
    return weather.windSpeed();
  });
  ascom.on(F("boltwood"), []() {
    ascom.success(weather.boltwoodData());
  });
}


// TODO:
// - wifi tcp/http connection, response?
// - serial ascom interface?
// - wind sensor
// - http send back bitmap for wind data

void setup() {
  Serial.begin(115200);
  while (!Serial); // Waiting for Serial Monitor
  Wire.begin();
  if (!weather.begin()) {
    Serial.println(weather.error);
    while (true) delay(10);
  }
  setupAscom();
  delay(1000);
}

void loop() {
  weather.loop();
  ascom.loop();
}

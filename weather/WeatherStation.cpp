#include "WeatherStation.h"
#include "src/Constants.h"
#include "src/DateFormatting.h"

#define READ_FREQ_SHT 300 * MICROS_PER_MS
#define READ_FREQ_BMP 500 * MICROS_PER_MS
#define READ_FREQ_TSL 7 * MICROS_PER_SEC
#define READ_FREQ_AMG 13 * MICROS_PER_SEC
#define BUCKET_FREQ_WIND 30 * MICROS_PER_SEC


WeatherStation::WeatherStation(
  TwoWire * wireInst,
  uint8_t rg11Pin,
  uint8_t mdWindPin
) : 
  wire(wireInst), 
  pin_rg11(rg11Pin),
  pin_wind(mdWindPin),
  timeClient(NTPClient(
    ntpUDP,
    "pool.ntp.org", // ntp server
    // CA is -7 hrs from UTC. TX is -5
    -7 * 3600, // offset, in seconds
    10 * 60 * 1000 // update interval, in milliseconds
  ))
{
  
}

boolean WeatherStation::begin() {
  boolean success = true;
  timeClient.begin();
  pinMode(pin_rg11, INPUT);

  /* SHT31-D Temperature & Humidity Sensor */
  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    error += F("No SHT31 sensor found...\n");
    success = false;
  }
  sht31.heater(false);

  /* TSL2591 Digital Light Sensor */
  if (!tsl.begin(&Wire, 0x29)) {
    error += F("No TSL2591 sensor found...\n");
    success = false;
  }
  tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain
  tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)

  /* BMP280 Barometric Pressure & Altitude Sensor */
  bmp = Adafruit_BMP280(&Wire);
  if (!bmp.begin()) {
    error += F("No BMP280 sensor found...");
    success = false;
  }

  /* Default settings from datasheet. */
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
    Adafruit_BMP280::STANDBY_MS_500   /* Standby time. */
  );

  /* AMG8833 IR Thermal Camera */
  if (!amg.begin()) {
    error += F("No AMG8833 sensor found...");
    success = false;
  }

  /* Modern Devices Wind Sensor Rev. P */
  if (!windSensor.begin(
    pin_wind
  )) {
    error += F("Wind Sensor initialization failed...");
    success = false;
  }
  return success;
}

void WeatherStation::loop() {
  if (micros() - tslLastRead > READ_FREQ_TSL) {
    tslLastRead = micros();
    tslLuminosity = tsl.getLuminosity(TSL2591_VISIBLE);
  }
  if (micros() - bmpLastRead > READ_FREQ_BMP) {
    bmpLastRead = micros();
    bmpTemperature = bmp.readTemperature();
    bmpPressure = bmp.readPressure();
    bmpAltitude = bmp.readAltitude(1013.25); /* Adjusted to local forecast! */
  }
  if (micros() - shtLastRead > READ_FREQ_SHT) {
    shtLastRead = micros();
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();

    if (!isnan(t)) {  // check if 'is not a number'
      shtTemperature = t;
    }
    
    if (!isnan(h)) {  // check if 'is not a number'
      shtHumidity = h;
    }
  }
  if (micros() - amgLastRead > READ_FREQ_AMG) {
    amgLastRead = micros();
    amg.readPixels(amgPixels);
  }
  // cheap enough to run every time
  rg11Value = digitalRead(pin_rg11);
  // cheap enough to run every time
  windSensor.loop();
  if (micros() - windLastBucket > BUCKET_FREQ_WIND) {
    windLastBucket = micros();
    for (int i = 0; i < GUST_BUCKETS - 1; i++) {
      windGusts[i] = windGusts[i + 1];
    }
    windGusts[GUST_BUCKETS - 1] = windSpeed();
  } else {
    windGusts[GUST_BUCKETS - 1] = max(windSpeed(), windGusts[GUST_BUCKETS-1]);
  }
}

float WeatherStation::percentCloudCover() {
  float ambientTemp = ambientTemperature();
  float skyTemp = skyTemperature();
  float dt = ambientTemp - skyTemp;

  // Calculate cloud cover
  float slope = (100 - 0) / (cloudyAmbientSkyDelta - clearAmbientSkyDelta);
  // ensure the cloud reported is between 0 and 100 percent
  float cloud = min(100 , max(0 , (int)(300 + slope * dt)));
  return cloud;
}

float WeatherStation::ambientTemperature() {
  return shtTemperature;
}

float WeatherStation::skyTemperature() {
  float agg = 0;
  for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
    agg += amgPixels[i];
  }
  return agg / AMG88xx_PIXEL_ARRAY_SIZE;
}

// https://en.wikipedia.org/wiki/Dew_point#Calculating_the_dew_point
// humidity is in percent, temp in celsius, result in celsius
float WeatherStation::dewPoint() {
  // Dew Point calculation
  // various different constants can be used with varying degrees of error.
  // these supposedly produce errors within 0.4 deg C for 0-60 deg C ambient temp
  float ambientTemp = ambientTemperature();
  float hum = humidity();
  float b = 17.271;
  float c = 237.7;
  float gamma = log(hum * 0.01) + (b * ambientTemp) / (c + ambientTemp);
  float dew = (c * gamma) / (b - gamma);
  return dew;
}

uint16_t WeatherStation::luminosity() {
  return tslLuminosity;
}

boolean WeatherStation::isRaining() {
  return rg11Value;
}

float WeatherStation::humidity() {
  return shtHumidity;
}

float WeatherStation::pressure() {
  return bmpPressure;
}

float WeatherStation::altitude() {
  return bmpAltitude;
}

float WeatherStation::windGust() {
  float gust = 0;
  for (int i = 0; i < GUST_BUCKETS; i++) {
    gust = max(windGusts[i], gust);
  }
  return gust;
}

float WeatherStation::windSpeed() {
  return windSensor.windSpeed(ambientTemperature());
}

String WeatherStation::boltwoodData() {
  unsigned long epoch = timeClient.getEpochTime();

  String cloudCoverFlag = "X";
  String darknessFlag = "X";

  return "" +
    getFormattedDate(epoch) + " " + // File write date
    getFormattedTime(epoch) + " " + // File write time

    "F " +  // Temperature scale (Celsius or Fahrenheit)
    "M " + // Wind speed scale (Mph or Knots)

    String(skyTemperature(), 1) + " " + // Sky Temperature
    String(ambientTemperature(), 1) + " " + // Ambient Temperature // TODO should we use bmp instead?
    String(ambientTemperature(), 1) + " " + // Sensor Temperature

    String(0.0, 1) + " " + // Wind Speed
    String(humidity(), 0) + " " + // Humidity
    String(dewPoint(), 1) + " " + // Dew Point

    "000 " +  // Dew Heater Percentage // TODO what is this?
    "0 " + // Rain Flag
    "0 " + // Wet Flag

    "00020" + " " +// Elapsed time since last file write (00020)
    "043117.61927" + " " +// Elapsed days since last write (043117.61927)
    
    cloudCoverFlag + " " + // Cloud/Clear flag (1=Clear,2=Light Clouds,3=Very Cloudy)
    1 + " " +// Wind Limit flag (1=Calm,2=Windy,3=Very Windy)
    1 + " " + // Rain flag (1=Dry,2=Damp,3=Rain)
    darknessFlag + " " + // Darkness flag (1=Dark,2=Dim,3=Daylight)

    0 + " " + // Roof Close flag
    0; // Alert flag (0=No Alert,1=Alert)
}



class MDWindSensor {
public:
  boolean begin(uint8_t pinWind) {
    pin_Wind = pinWind;
    pinMode(pin_Wind, INPUT);
    return true;
  }
  void loop() {
    // TODO: calculate gusts
    // Right now I'm using the temp sensor on the wind gauge itself, mostly because i suppose that is the
    // sensor that it was calibrated with, but we should consider using one of the other sensors we have
    // available since it might be more accurate (and more importantly, it will be out of direct sun exposure)
    // float temp_adu = analogRead(pin_Temp);
    // float temp_voltage = temp_adu * .0049f; // 0-5V ~ 0-1024 ADU -> .0049 V / ADU

    // // voltage -> celsius calculation for temp sensor
    // float temp_ambient = (temp_voltage - 0.4f) / 0.0195f; // deg C

    // // constrain to reasonable range to avoid deviating from calibration too much and potential divide by zero
    // temp_ambient = min(max(temp_ambient, 10.0f), 50.0f);

    // currentTemp = temp_ambient;

    windAdu = analogRead(pin_Wind);
    
  }

  float windSpeed(float ambientTemp) {
    float wind_voltage = (float)windAdu * .0049f;

    // apply voltage offset and make sure not negative
    // by default the voltage offset is the number provide by the manufacturer
    float adj_wind_voltage = wind_voltage - zeroWindVoltave;
    if (adj_wind_voltage <= 0.0f) {
        adj_wind_voltage = 0.0f;
    }

    // wind speed calculation
    // https://moderndevice.com/news/calibrating-rev-p-wind-sensor-new-regression/
    return powf((adj_wind_voltage / powf(ambientTemp, 0.115157f)), 3.009364f);
  }

private:
  uint8_t pin_Wind;

  // negative numbers yield smaller wind speeds and vice versa.
  float zeroWindVoltave = .2;
  int windAdu;
};
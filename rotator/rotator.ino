// #include "src/SerialCommander.h"
#include "src/AscomApi.h"
#include "src/Disk.h"
#include "Motor.h"
#include <EEPROM.h>

#define STEPS 200
#define MIN_ADDR 0
#define MAX_ADDR MIN_ADDR + sizeof(int)


#ifndef WIFI_CREDENTIALS
#define WIFI_CREDENTIALS
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PWD "your-wifi-pwd"
#endif


const char *ssid = WIFI_SSID;
const char *pwd = WIFI_PWD;

AscomApi ascom = AscomApi();
Motor motor(STEPS, 25, 26, 27, 15);

void setupAscomInterface() {
  ascom.begin("rotator", ssid, pwd);

  ascom.propertyInt(F("POSITION"), []() {
    return motor.position();
  });

  ascom.propertyInt(F("ISMOVING"), []() {
    int moving = 0;
    if (motor.isMoving()) {
      moving = 1;
    }
    return moving;
  });

  ascom.propertyInt(F("MIN"), []() {
    return motor.getMin();
  }, [](int min) {
    motor.setMin(min);
  });

  ascom.propertyInt(F("MAX"), []() {
    return motor.getMax();
  }, [](int max) {
    motor.setMax(max);
  });

  ascom.command(F("GOTO"), []() {
    int position = ascom.argInt(0);
    motor.goTo(position);
  });

  ascom.command(F("STOP"), []() {
    motor.stop();
  });

  ascom.command(F("SAVE"), []() {
    motor.stop();
    saveRange();
  });

  ascom.command(F("RESTART"), []() {
    saveRange();
    delay(500);
    esp_restart();
  });

  ascom.onNotFound([]() {
    ascom.response(404, F("UNKNOWN COMMAND"));
  });
}


void saveRange() {
  int min = motor.getMin();
  int max = motor.getMax();
  int position = motor.position();
  writeInt(MIN_ADDR, min - position);
  writeInt(MAX_ADDR, max - position);
  EEPROM.commit();
}

void loadRange() {
  int min = readInt(MIN_ADDR);
  int max = readInt(MAX_ADDR);
  motor.setRange(min, max);
}

void setup()
{
  Serial.begin(115200);
  while(!Serial) {
    delay(100);
  }
  Serial.println("HELLO WORLD");
  loadRange();
  setupAscomInterface();
  delay(1000);
}

void loop()
{
  ascom.loop();
  motor.loop();
}
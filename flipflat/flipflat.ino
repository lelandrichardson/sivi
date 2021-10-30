#include "src/SerialCommander.h"

SerialCommander ascom = SerialCommander();

void setupSerialInterface() {
  Serial.begin(115200);
  while (!Serial);
  ascom.begin();

  ascom.on(F("NAME"), []() {
    ascom.success("flipflat");
  });

  ascom.on(F("ISOPEN"), []() {
    ascom.success(motor.isOpen());
  });

  ascom.on(F("ISLIGHTON"), []() {
    ascom.success(motor.isLightOn());
  });

  ascom.on(F("OPEN"), []() {
    motor.open();
    ascom.success();
  });

  ascom.on(F("CLOSE"), []() {
    motor.close();
    ascom.success();
  });

  ascom.on(F("LIGHT"), []() {
    int on = ascom.argInt(0);
    if (on == 0) {
      motor.turnLightOff();
    } else {
      motor.turnLightOn();
    }
    ascom.success();
  });

  ascom.on(F("RESTART"), []() {
    ascom.success();
    delay(500);
    esp_restart();
  });

  ascom.onNotFound([]() {
    ascom.response(404, F("UNKNOWN COMMAND"));
  });
}

void setup()
{
  setupSerialInterface();
}

void loop()
{
  ascom.loop();
}
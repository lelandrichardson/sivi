#include "src/SerialCommander.h"
#include "Motor.h"
// TODO:
// - serial interface, ascom
// - http interface?
// - ir sensors, implement boundaries and position finding
// - store position/min/max in memory

SerialCommander ascom = SerialCommander();
Motor motor = Motor();

void setupSerialInterface() {
  Serial.begin(115200);
  while (!Serial);
  ascom.begin();

  ascom.on(F("NAME"), []() {
    ascom.success("rotator");
  });

  ascom.on(F("POSITION"), []() {
    ascom.success(motor.positioon());
  });

  ascom.on(F("ISMOVING"), []() {
    ascom.success(motor.isMoving());
  });

  ascom.on(F("RANGE"), []() {
    int min = motor.min();
    int max = motor.max();
    ascom.success();
  });

  ascom.on(F("GOTO"), []() {
    int position = ascom.argInt(0);
    motor.goTo(position);
    ascom.success();
  });

  ascom.on(F("STOP"), []() {
    motor.stop();
    ascom.success();
  });

  ascom.on(F("SETRANGE"), []() {
    int min = ascom.argInt(0);
    int max = ascom.argMax(1);
    motor.setRange(min, max);
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
  setupSerialInterface()
  motor.begin();
}

void loop()
{
  ascom.loop();
  motor.loop();
}
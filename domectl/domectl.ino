// SIVI Dome Control
// =================
#include <WiFi.h>
#include "src/Display.h"
#include "src/SerialCommander.h"
#include "DomeMotor.h"
#include "ShutterClient.h"

// WiFiServer server(80);


// =======================================================
// PIN CONFIGURATION
// Heltec ESP32 WiFi Kit 32
// Pinout: https://resource.heltec.cn/download/WiFi_Kit_32/WIFI_Kit_32_pinoutDiagram_V2.pdf
// =======================================================
#define UNASSIGNED 99
const uint8_t PIN_DOME_MTR_R_PWM = 17;
const uint8_t PIN_DOME_MTR_L_PWM = 5;
const uint8_t PIN_ENCODER_A = 14;
const uint8_t PIN_ENCODER_B = 27;
const uint8_t PIN_HOME_POS_SENSOR = UNASSIGNED;
const uint8_t PIN_DISPLAY_WAKE_SWITCH = 18;
const uint8_t PIN_MTR_SWITCH_R = 23;
const uint8_t PIN_MTR_SWITCH_L = 19;
const uint8_t PIN_SHUTTER_SWITCH = UNASSIGNED;
const uint8_t PIN_FIND_HOME_SWITCH = UNASSIGNED;

const uint8_t PIN_SHUTTER_OPEN_SENSOR = UNASSIGNED;
const uint8_t PIN_SHUTTER_CLOSED_SENSOR = UNASSIGNED;

Display display = Display();
SerialCommander ascom = SerialCommander();
DomeMotor motor = DomeMotor(
  PIN_DOME_MTR_R_PWM,
  PIN_DOME_MTR_L_PWM
);
ShutterClient shutter = ShutterClient();

void setupSerialInterface() {
  Serial.begin(115200);
  while (!Serial);
  ascom.begin();

  ascom.on(F("NAME"), []() {
    ascom.success("domectl");
  });

  ascom.on(F("ATHOME"), []() {
    // true when the dome is in the home position. Raises an error if not supported. 
    // 
    // This is normally used following a FindHome()  operation. The value is reset 
    // with any azimuth slew operation that moves the dome away from the home position.
    // 
    // AtHome may optionally also become true during normal slew operations, if the dome 
    // passes through the home position and the dome controller hardware is capable of 
    // detecting that; or at the end of a slew operation if the dome comes to rest at the 
    // home position.  
    ascom.success(motor.isHome());
  });
  ascom.on(F("ATPARK"), []() {
    // true if the dome is in the programmed park position.
    ascom.success(motor.isParked());
  });
  ascom.on(F("AZIMUTH"), []() {
    // The dome azimuth (degrees, North zero and increasing clockwise, i.e., 90 East, 
    // 180 South, 270 West). North is true north and not magnetic north.
    ascom.success(motor.getPositionInDegrees(), 3);
  });
  ascom.on(F("SHUTTERSTATUS"), []() {
    // Gets the status of the dome shutter or roof structure.
    ascom.success(shutter.statusString());
  });
  ascom.on(F("SLEWING"), []() {
    // true if any part of the dome is currently moving, false if all dome components are 
    // stationary. 
    boolean slewing = motor.isSlewing();
    ascom.success(motor.isSlewing());
  });
  ascom.on(F("ABORTSLEW"), []() {
    // Immediately stops any and all movement.
    motor.stop();
    ascom.success();
  });
  ascom.on(F("OPENSHUTTER"), []() {
    // Open shutter or otherwise expose telescope to the sky.
    shutter.open();
    ascom.success();
  });
  ascom.on(F("CLOSESHUTTER"), []() {
    // Close the shutter or otherwise shield the telescope from the sky.
    shutter.close();
    ascom.success();
  });
  ascom.on(F("FINDHOME"), []() {
    // Start operation to search for the dome home position.
    motor.slewToHome();
    ascom.success();
  });
  ascom.on(F("PARK"), []() {
    // Rotate dome in azimuth to park position.
    motor.slewToPark();
    ascom.success();
  });
  ascom.on(F("SETPARK"), []() {
    // Set the current azimuth position of dome to the park position.
    motor.setPark();
    ascom.success();
  });
  ascom.on(F("SLEWTOAZIMUTH"), []() {
    // Ensure that the requested viewing azimuth is available for observing. The method should 
    // not block and the slew operation should complete asynchronously. 
    float azimuth = ascom.argFloat(0);
    motor.slewTo(azimuth);
    ascom.success();
  });
  ascom.on(F("SYNCTOAZIMUTH"), []() {
    // Synchronize the current position of the dome to the given azimuth.
    float azimuth = ascom.argFloat(0);
    motor.syncCurrentPositionAsDegrees(azimuth);
    ascom.success();
  });
  
  ascom.on(F("RESTART"), []() {
    ascom.success();
    delay(500);
    esp_restart();
  });
  // TODO: routine to find and update "stepsPerDomeRotation"
  
  // TODO: update motor configurations
  // - pid constants
  // - steps per dome rotation or dome size / wheel size
  // - minimum error distance
  // - homePositionInSteps?
  // - stepsToTrueNorthOffset?
  ascom.onNotFound([]() {
    ascom.response(404, F("UNKNOWN COMMAND"));
  });
}

void encoderChangeInterrupt() {
  boolean A = digitalRead(PIN_ENCODER_A);
  boolean B = digitalRead(PIN_ENCODER_B);
  motor.encoderPulse(A, B);
}

void motorButtonPressed() {
  display.wake();
  boolean leftPressed = digitalRead(PIN_MTR_SWITCH_L);
  boolean rightPressed = digitalRead(PIN_MTR_SWITCH_R);
  if (leftPressed && !rightPressed) {
    motor.slewLeft();
  } else if (rightPressed && !leftPressed) {
    motor.slewRight();
  } else {
    motor.stop();
  }
}

void setupShutter() {
  shutter.begin(
    "ssid",
    "passphrase"
  );
}

void setupDomeMotor() {
  pinMode(PIN_MTR_SWITCH_R, INPUT_PULLUP);
  pinMode(PIN_MTR_SWITCH_L, INPUT_PULLUP);
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  motor.begin(
    digitalRead(PIN_ENCODER_A),
    digitalRead(PIN_ENCODER_B)
  );
  attachInterrupt(PIN_MTR_SWITCH_R, motorButtonPressed, CHANGE);
  attachInterrupt(PIN_MTR_SWITCH_L, motorButtonPressed, CHANGE);
  attachInterrupt(PIN_ENCODER_A, encoderChangeInterrupt, CHANGE);
  attachInterrupt(PIN_ENCODER_B, encoderChangeInterrupt, CHANGE);
}


const int LeftCol = 45;
const int MiddleCol = 80;
String DEGREE = "°";

void displayLabeledValue(String label, String value, String units) {
  display.text(Right, LeftCol, "Position:");
  display.text(Right, MiddleCol, String(motor.getPositionInDegrees(), 1));
  display.text(Left, MiddleCol, DEGREE);
  display.nextLine();
}

void wakeSleepButtonPressed() {
  display.toggle();
}

void setupDisplay() {
  pinMode(PIN_DISPLAY_WAKE_SWITCH, INPUT_PULLUP);
  attachInterrupt(PIN_DISPLAY_WAKE_SWITCH, wakeSleepButtonPressed, FALLING);
  display.begin();
  display.onDraw([]() {
    // Example Display Readout:
    // ==================================
    // | Position: 156.6°          28 |
    // | Velocity:  46.6° / min       |
    // |     Home: 156.6° (current)   |
    // |     Park:  16.3°             |
    // |  Shutter: Opening            |
    // | -     GETAZIMUTH         4 - |
    // ==================================

    displayLabeledValue(
      F("Position:"),
      String(motor.getPositionInDegrees(), 1),
      DEGREE
    );
    displayLabeledValue(
      F("Velocity:"),
      String(motor.getVelocityInDegreesPerSec(), 2),
      DEGREE + " / sec"
    );
    displayLabeledValue(
      F("Home:"),
      "32.2",
      DEGREE
    );
    displayLabeledValue(
      F("Park:"),
      "32.2",
      DEGREE + " (current)"
    );

    display.text(Right, LeftCol, "Shutter:");
    display.text(Right, LeftCol + 10, "Opening");
    display.text(Left, MiddleCol, DEGREE);
  });
}

void setup()
{
  setupDisplay();
  setupSerialInterface();
  setupShutter();
  setupDomeMotor();
}

void loop()
{
  ascom.loop();
  shutter.loop();
  motor.loop();
  display.loop();
}
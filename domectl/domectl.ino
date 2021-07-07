

// SIVI Dome Control
// =================

// TODO:
// - tcp connection with shutter
// - PID for motor

// Goals:
// 1. Display
//    a. momentary switch turns OLED display on/off
//    b. OLED display shows current details + debug info
// 2. Dome Rotation Position / Encoder
//    a. incremental encoder from 2 photo interruptors
//    b. "home" position detected with magnetic switch
// 3. Serial interface
//    a. "ATHOME" - if dome is at home position
//    b. "ATPARK" - if dome is at park position
//    c. "AZIMUTH" - current rotational position in degrees
//    d. "CONNECTED" - ping response
//    e. "SHUTTERSTATUS" - status of shutter
//    f. "SLEWING" - if dome is currently moving
//    g. "ABORTSLEW"
//    h. "CLOSESHUTTER"
//    i. "FINDHOME"
//    j. "PARK"
//    k. "SETPARK"
//    l. "SLEWTOAZIMUTH"
//    m. "SYNCTOAZIMUTH"
// 4. Manual Interface
//    a. momentary switch to rotate right
//    b. momentary switch to rotate left
//    c. momentary switch to find home
//    d. momentary switch to open/close shutter
//    e. momentary switch to turn OLED display on/off
// 3. Dome motor control
//    a. 

boolean isSlewing = false;
long knownHomePosition = 0;
boolean isAtHome = false;
boolean isFindingHome = false;
boolean isClosingShutter = false;
boolean isOpeningShutter = false;
float slewTargetInDegrees = 0;



#include "heltec.h"
#include <analogWrite.h>


#define MICROS_PER_MS 1000
#define MICROS_PER_SEC 1000000
#define MILLIS_PER_SEC 1000
#define ENCODER_STEPS_PER_ROTATION 2400


#define SLEEP_ENABLED true







#define DEBOUNCE_MICROS 300 * MICROS_PER_MS


















// =======================================================
// PIN CONFIGURATION
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
const uint8_t PIN_SHUTTER_OPEN_SENSOR = UNASSIGNED;
const uint8_t PIN_SHUTTER_CLOSED_SENSOR = UNASSIGNED;
const uint8_t PIN_FIND_HOME_SWITCH = UNASSIGNED;


























// =======================================================
// Serial Interface
// =======================================================

String serialCommandString = "";         // a String to hold incoming data
boolean pendingSerialCommand = false;  // whether the string is complete

void setupSerialInterface() {
  Serial.begin(115200);
  while (!Serial);
  // reserve 200 bytes for the serial buffer
  serialCommandString.reserve(200);
}

void loopSerialInterface() {
  if (pendingSerialCommand) {
    // TODO: process
    
    pendingSerialCommand = false;
    serialCommandString = "";
  }
}


/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    serialCommandString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      pendingSerialCommand = true;
    }
  }
}


































































// =======================================================
// Rotary Encoder
// =======================================================

volatile long encoderPosition = 0;
volatile uint8_t encoderState = 0;
unsigned long lastVelocityTime;
long lastVelocityPosition;
// velocity in encoder steps per microsecond
float currentVelocity = 0.0;

#define WHEEL_DIAMETER_INCHES 2.975
#define DOME_DIAMETER_INCHES 120.2
// It's unclear to me if we should just figure out what this number is and make it a constant, or have the software update this as it
// goes. It's probably not worth updating it since the dome will rarely undergo multiple rotations in the course of a night.
unsigned long stepsPerDomeRotation = ENCODER_STEPS_PER_ROTATION * DOME_DIAMETER_INCHES / WHEEL_DIAMETER_INCHES; // PI cancels out


// STATES (AB):
// 11 = 0
// 10 = 1
// 00 = 2
// 01 = 3

// Quadrature Encoder Matrix:
// Row = prev state
// Col = next state
// Val = increase (1), decrease (-1), stay the same (0)

volatile long QEM[16] = {
  0, -1, 0, 1,
  1, 0, -1, 0,
  0, 1, 0, -1,
  -1, 0, 1, 0,
};

void encoderChangeInterrupt() {
  uint8_t nextState = readEncoderState();
  uint8_t index = 4*nextState + encoderState;
  encoderPosition = encoderPosition + QEM[index];
  encoderState = nextState;
}

uint8_t readEncoderState() {
  boolean A = digitalRead(PIN_ENCODER_A);
  boolean B = digitalRead(PIN_ENCODER_B);
  if (A == HIGH && B == HIGH) return 0;
  if (A == HIGH && B == LOW) return 1;
  if (A == LOW && B == LOW) return 2;
  if (A == LOW && B == HIGH) return 3;
  return 0;
}


void setupRotaryEncoding() {
  encoderPosition = 0; // TODO: save/restore?
  currentVelocity = 0;
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  lastVelocityTime = micros();
  encoderState = readEncoderState();
  attachInterrupt(PIN_ENCODER_A, encoderChangeInterrupt, CHANGE);
  attachInterrupt(PIN_ENCODER_B, encoderChangeInterrupt, CHANGE);
}

void loopRotaryEncoding() {
  unsigned long currentTime = micros();
  unsigned long encTimeElapsed = currentTime - lastVelocityTime;
  // update velocity once per second, keep track of 8 seconds worth of data
  // for a weighted average
  if (encTimeElapsed > 500 * MICROS_PER_MS) {
    long positionDelta = encoderPosition - lastVelocityPosition;
    currentVelocity = (float)positionDelta / encTimeElapsed;
    lastVelocityPosition = encoderPosition;
    lastVelocityTime = currentTime;
  }
}

























// =======================================================
// Dome Motor Control
// =======================================================

volatile int _motorVoltageTarget = 0;
int currentMotorVoltage = 0;


void motorButtonPressed() {
  wakeDisplay();
  boolean leftPressed = digitalRead(PIN_MTR_SWITCH_L);
  boolean rightPressed = digitalRead(PIN_MTR_SWITCH_R);
  if (leftPressed && !rightPressed) {
    _motorVoltageTarget = -255;
  } else if (rightPressed && !leftPressed) {
    _motorVoltageTarget = 255;
  } else {
    _motorVoltageTarget = 0;
  }
}

void setupDomeMotor() {
  pinMode(PIN_DOME_MTR_R_PWM, OUTPUT);
  pinMode(PIN_DOME_MTR_L_PWM, OUTPUT);
  pinMode(PIN_MTR_SWITCH_R, INPUT_PULLUP);
  pinMode(PIN_MTR_SWITCH_L, INPUT_PULLUP);
  attachInterrupt(PIN_MTR_SWITCH_R, motorButtonPressed, CHANGE);
  attachInterrupt(PIN_MTR_SWITCH_L, motorButtonPressed, CHANGE);
}

void loopDomeMotor() {
  delay(50);
  int voltageTarget = _motorVoltageTarget;
  if (currentMotorVoltage < voltageTarget) {
    currentMotorVoltage = min(voltageTarget, currentMotorVoltage + 10);
  } else if (currentMotorVoltage > voltageTarget) {
    currentMotorVoltage = max(voltageTarget, currentMotorVoltage - 10);
  }
  if (currentMotorVoltage >= 0) {
    analogWrite(PIN_DOME_MTR_L_PWM, 0);
    analogWrite(PIN_DOME_MTR_R_PWM, currentMotorVoltage);
  } else if (currentMotorVoltage < 0) {
    analogWrite(PIN_DOME_MTR_L_PWM, -currentMotorVoltage);
    analogWrite(PIN_DOME_MTR_R_PWM, 0);
  }
}






















































// =======================================================
// OLED Display
// =======================================================


const unsigned long MICROS_UNTIL_DISPLAY_SLEEP = 30 * MICROS_PER_SEC;
const int LINE_HEIGHT = 10;
const int LeftCol = 45;
const int MiddleCol = 80;
const int RightCol = 128;
String DEGREE = "°";

unsigned long displayLastRefreshTime;
volatile boolean displayIsSleeping = false;
volatile unsigned long displayWakeTime;
unsigned long displayToastMessageShown;
String displayToastMessage;




void drawStringAlignRight(int16_t alignX, int16_t lineN, String msg) {
  int16_t msgWidth = Heltec.display -> getStringWidth(msg);
  int16_t deltaX = max(0, alignX - msgWidth);
  Heltec.display -> drawString(deltaX, lineN * LINE_HEIGHT, msg);
}

void drawStringAlignLeft(int16_t alignX, int16_t lineN, String msg) {
  Heltec.display -> drawString(alignX, lineN * LINE_HEIGHT, msg);
}

void drawStringAlignCenter(int16_t left, int16_t right, int16_t lineN, String msg) {
  int16_t msgWidth = Heltec.display -> getStringWidth(msg);
  int16_t startX = max(0, (left + right - msgWidth) / 2);
  Heltec.display -> drawString(startX, lineN * LINE_HEIGHT, msg);
}

void toast(String msg) {
  displayToastMessage = msg;
  displayToastMessageShown = micros();
}

void wakeDisplay() {
  displayIsSleeping = false;
  displayWakeTime = micros();
}

void wakeSleepButtonPressed() {
  unsigned long currentTime = micros();
  unsigned long microsSinceWake = currentTime - displayWakeTime;
  if (microsSinceWake < DEBOUNCE_MICROS) {
    // the display was just woken up. return early to avoid flicker.
    return;
  }
  if (microsSinceWake - MICROS_UNTIL_DISPLAY_SLEEP < DEBOUNCE_MICROS) {
    // the display was just put to sleep. return early to avoid flicker.
    return;
  }
  if (microsSinceWake > MICROS_UNTIL_DISPLAY_SLEEP) {
    // wake up display
    displayWakeTime = currentTime;
  } else {
    // put display to sleep
    displayWakeTime = currentTime - MICROS_UNTIL_DISPLAY_SLEEP - 1;
  }
  displayIsSleeping = false;
}

void setupDisplay() {
  // enable display wake/sleep button
  pinMode(PIN_DISPLAY_WAKE_SWITCH, INPUT_PULLUP);
  attachInterrupt(PIN_DISPLAY_WAKE_SWITCH, wakeSleepButtonPressed, FALLING);

  // refresh time
  Heltec.begin(
    true /*DisplayEnable Enable*/, 
    false /*LoRa Enable*/, 
    false /*Serial Enable*/ // NOTE: Serial connection is enabled elsewhere
  );
  Heltec.display -> clear();
  Heltec.display -> drawString(0, 0, "SIVI Dome Control");
  displayLastRefreshTime = micros();
  displayWakeTime = micros();
  Heltec.display -> display();
  delay(1000);
}

void loopDisplay() {
  if (displayIsSleeping) return;

  unsigned long currentTime = micros();
  unsigned long microsSinceWake = currentTime - displayWakeTime;
  unsigned long timeSinceLastRefresh = currentTime - displayLastRefreshTime;

  if (SLEEP_ENABLED && microsSinceWake > MICROS_UNTIL_DISPLAY_SLEEP) {
    displayIsSleeping = true;
    Heltec.display -> clear();
    Heltec.display -> display();
    return;
  }


  if (timeSinceLastRefresh > 100 * MICROS_PER_MS) {
    displayLastRefreshTime = currentTime;

    // update display 2 times / second
    Heltec.display -> clear();

    // Example Display Readout:
    // ==================================
    // | Position: 156.6°          28 |
    // | Velocity:  46.6° / min       |
    // |     Home: 156.6° (current)   |
    // |     Park:  16.3°             |
    // |  Shutter: Opening            |
    // | -     GETAZIMUTH         4 - |
    // ==================================
    
    // display will go to sleep after 30s. 
    // Display time left in the top right
    int secondsUntilSleep = (MICROS_UNTIL_DISPLAY_SLEEP - microsSinceWake) / MICROS_PER_SEC;
    char buf2[8];
    dtostrf(secondsUntilSleep, 3, 0, buf2);
    drawStringAlignRight(RightCol, 0, buf2);

    // Position (in degrees)
    drawStringAlignRight(LeftCol, 0, "Position:");
    // position, in wheel revolutions
    float angularPosition = ((float) encoderPosition) / stepsPerDomeRotation * 360.0;
    // char posbuf[8];
    // sprintf(posbuf, "%d -", encoderPosition);
    drawStringAlignRight(MiddleCol, 0, String(angularPosition, 1));
    drawStringAlignLeft(MiddleCol, 0, DEGREE);

    // Velocity (in degrees / min)
    // currentVelocity, in steps per microsecond
    // angVelocity, in degrees per second
    float angVelocity = currentVelocity * MICROS_PER_SEC * 360 / stepsPerDomeRotation;

    drawStringAlignRight(LeftCol, 1, "Velocity:");
    drawStringAlignRight(MiddleCol, 1, String(angVelocity, 2));
    drawStringAlignLeft(MiddleCol, 1, DEGREE + " / sec");

    // Home position
    drawStringAlignRight(LeftCol, 2, "Home:");
    drawStringAlignRight(MiddleCol, 2, "32.2");
    drawStringAlignLeft(MiddleCol, 2, DEGREE);

    // Park position
    drawStringAlignRight(LeftCol, 3, "Park:");
    drawStringAlignRight(MiddleCol, 3, "32.2");
    drawStringAlignLeft(MiddleCol, 3, DEGREE + " (current)");

    // Shutter status
    drawStringAlignRight(LeftCol, 4, "Shutter:");
    drawStringAlignLeft(LeftCol + 10, 4, "Opening");

    unsigned long timeSinceToastMessage = currentTime - displayToastMessageShown;

    if (timeSinceToastMessage < 5 * MICROS_PER_SEC) {
      // Debug messaging. Message active for 5 seconds.
      int toastSecsLeft = (5 * MICROS_PER_SEC - timeSinceToastMessage) / MICROS_PER_SEC;
      drawStringAlignLeft(0, 5, "-");
      char buf[8];
      sprintf(buf, "%d -", toastSecsLeft);
      drawStringAlignRight(RightCol, 5, buf);
      drawStringAlignCenter(0, RightCol, 5, "GETAZIMUTH");
    }


    Heltec.display -> display();
  }
}



























































// =======================================================
// Main Program
// =======================================================

void setup()
{
  setupDisplay();
  setupSerialInterface();
  setupRotaryEncoding();
  setupDomeMotor();
}

void loop()
{
  loopDisplay();
  loopSerialInterface();
  loopRotaryEncoding();
  loopDomeMotor();
}
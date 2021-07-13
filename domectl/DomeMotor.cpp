#include "DomeMotor.h"

#define ENCODER_STEPS_PER_ROTATION 2400
#define WHEEL_DIAMETER_INCHES 2.975
#define DOME_DIAMETER_INCHES 120.2

DomeMotor::DomeMotor(uint8_t rightPin, uint8_t leftPin) : 
  pin_pwm_r(rightPin), 
  pin_pwm_l(leftPin), 
  pid(PID(
    &input,
    &output,
    &setPoint,
    // TODO: make these tweakable
    kp,
    ki,
    kd,
    DIRECT
  )) 
{
  
}

boolean DomeMotor::begin(boolean A, boolean B) {
  // TODO: make this updateable
  // It's unclear to me if we should just figure out what this number is and make it a constant, or have the software update this as it
  // goes. It's probably not worth updating it since the dome will rarely undergo multiple rotations in the course of a night.
  stepsPerDomeRotation = ENCODER_STEPS_PER_ROTATION * DOME_DIAMETER_INCHES / WHEEL_DIAMETER_INCHES; // PI cancels out
  automatic = false;
  encoderPosition = 0; // TODO: save/restore?
  currentVelocity = 0;
  lastVelocityTime = micros();
  encoderState = stateOf(A, B);
  pinMode(pin_pwm_r, OUTPUT);
  pinMode(pin_pwm_l, OUTPUT);

  // TODO: make configurable
  minimumErrorDistance = 1000;
  // TODO: make pid constants tweakable
  // TODO: load constants and other values from EEPROM?
  // TODO: make sure we use motor pins that won't move motor during reset
  pid.SetMode(automatic);
  pid.SetControllerDirection(DIRECT); // DIRECT or REVERSE
  pid.SetOutputLimits(-255, 255);
  pid.SetSampleTime(200); // 200ms
  
  return true;
}

void DomeMotor::loop() {
  unsigned long currentTime = micros();
  unsigned long encTimeElapsed = currentTime - lastVelocityTime;

  // update velocity 2x per second
  if (encTimeElapsed > 500 * MICROS_PER_MS) {
    long positionDelta = encoderPosition - lastVelocityPosition;
    currentVelocity = (float)positionDelta / encTimeElapsed;
    lastVelocityPosition = encoderPosition;
    lastVelocityTime = currentTime;
  }

  input = (double)equivalentSteps(encoderPosition, (long)setPoint, stepsPerDomeRotation);

  if (pid.Compute()) {
    setMotorSpeed(output);
  }

  if (!automatic) {
    // update voltage sent to motor
    int voltageTarget = _motorVoltageTarget;
    if (currentMotorVoltage < voltageTarget) {
      currentMotorVoltage = min(voltageTarget, currentMotorVoltage + 10);
    } else if (currentMotorVoltage > voltageTarget) {
      currentMotorVoltage = max(voltageTarget, currentMotorVoltage - 10);
    }
    // TODO: we need to create some sort of a debounce timer here to only
    // update the manual motor voltage once per some time interval
    setMotorSpeed(currentMotorVoltage);
  }
}

void DomeMotor::setMotorSpeed(double speed) {
  if (speed >= 0) {
      analogWrite(pin_pwm_l, 0);
      analogWrite(pin_pwm_r, (uint32_t)speed);
    } else if (speed < 0) {
      analogWrite(pin_pwm_l, (uint32_t)(-1 * speed));
      analogWrite(pin_pwm_r, 0);
    }
}

long DomeMotor::getPositionInSteps() {
  return encoderPosition;
}

float DomeMotor::getPositionInDegrees() {
  // position, in wheel revolutions
  float angularPosition = ((float) encoderPosition) / stepsPerDomeRotation * 360.0;
}

float DomeMotor::getVelocityInDegreesPerSec() {
  // Velocity (in degrees / min)
  // currentVelocity, in steps per microsecond
  // angVelocity, in degrees per second
  return currentVelocity * MICROS_PER_SEC * 360 / stepsPerDomeRotation;
}

float DomeMotor::getVelocityInStepsPerSec() {
  // currentVelocity, in steps per microsecond
  return currentVelocity * MICROS_PER_SEC;
}

boolean DomeMotor::isSlewing() {
  if (_motorVoltageTarget != 0 || abs(output) > 1) {
    return false;
  }
  return true;
}

boolean DomeMotor::isParked() {
  return !isSlewing() && isEffectivelyAtPosition(parkPositionInSteps);
}

boolean DomeMotor::isHome() {
  return !isSlewing() && isEffectivelyAtPosition(homePositionInSteps);
}

float DomeMotor::getHomePositionInDegrees() {
  return stepsToDegrees(homePositionInSteps);
}

long DomeMotor::getHomePositionInSteps() {
  return homePositionInSteps;
}

void DomeMotor::syncCurrentPositionAsDegrees(float degrees) {
  
  // TODO
}

void DomeMotor::setPark(float degrees) {
  setPark(degreesToSteps(degrees));
}

void DomeMotor::setPark() {
  setPark(encoderPosition);
}

void DomeMotor::setPark(long steps) {
  parkPositionInSteps = steps;
}

void DomeMotor::slewTo(float degrees) {
  slewTo(degreesToSteps(degrees));
}

void DomeMotor::slewTo(long steps) {
  automaticMode();
  setPoint = nearestEquivalentSteps(steps);
}

void DomeMotor::slewToHome() {
  automaticMode();
  setPoint = nearestEquivalentSteps(homePositionInSteps);
}

void DomeMotor::slewToPark() {
  automaticMode();
  setPoint = nearestEquivalentSteps(parkPositionInSteps);
}

void DomeMotor::slewRight() {
  manualMode();
  _motorVoltageTarget = 255;
}

void DomeMotor::slewLeft() {
  manualMode();
  _motorVoltageTarget = -255;
}

void DomeMotor::stop() {
  manualMode();
  _motorVoltageTarget = 0;
}

void DomeMotor::manualMode() {
  if (automatic) {
    automatic = false;
    _motorVoltageTarget = 0;
    pid.SetMode(automatic);
  }
}

void DomeMotor::automaticMode() {
  if (!automatic) {
    automatic = true;
    _motorVoltageTarget = 0;
    pid.SetMode(automatic);
  }
}

boolean DomeMotor::isEffectivelyAtPosition(long steps) {
  long a = steps % stepsPerDomeRotation;
  long b = encoderPosition % stepsPerDomeRotation;
  return abs(a - b) < minimumErrorDistance;
}

long DomeMotor::degreesToSteps(float degrees) {
  return (long)(degrees / 360.0 * stepsPerDomeRotation);
}

float DomeMotor::stepsToDegrees(long steps) {
  double fractionRotated = (double)((steps % stepsPerDomeRotation)) / stepsPerDomeRotation;
  return fractionRotated * 360.0;
}

long DomeMotor::equivalentSteps(long steps, long relativeTo, long mod) {
  long pos = relativeTo % mod;
  long target = steps % mod;
  long other;
  if (target > pos) {
    other = target - mod;
  } else {
    other = target + mod;
  }

  if (abs(pos - target) < abs(pos - other)) {
    return target;
  } else {
    return other;
  }
}

long DomeMotor::nearestEquivalentSteps(long steps) {
  return equivalentSteps(steps, encoderPosition, stepsPerDomeRotation);
}

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

uint8_t DomeMotor::stateOf(boolean A, boolean B) {
  if (A == HIGH && B == HIGH) return 0;
  if (A == HIGH && B == LOW) return 1;
  if (A == LOW && B == LOW) return 2;
  if (A == LOW && B == HIGH) return 3;
  return 0;
}

void DomeMotor::encoderPulse(boolean A, boolean B) {
  uint8_t nextState = stateOf(A, B);
  uint8_t index = 4*nextState + encoderState;
  encoderPosition = encoderPosition + QEM[index];
  encoderState = nextState;
}
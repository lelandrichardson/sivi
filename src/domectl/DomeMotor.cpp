#include "DomeMotor.h"

boolean DomeMotor::begin(boolean A, boolean B) {
  encoderPosition = 0; // TODO: save/restore?
  currentVelocity = 0;
  lastVelocityTime = micros();
  encoderState = stateOf(A, B);
  pinMode(pin_pwm_r, OUTPUT);
  pinMode(pin_pwm_l, OUTPUT);
  return true;
}

void DomeMotor::loop() {
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

  // update voltage sent to motor
  int voltageTarget = _motorVoltageTarget;
  if (currentMotorVoltage < voltageTarget) {
    currentMotorVoltage = min(voltageTarget, currentMotorVoltage + 10);
  } else if (currentMotorVoltage > voltageTarget) {
    currentMotorVoltage = max(voltageTarget, currentMotorVoltage - 10);
  }
  if (currentMotorVoltage >= 0) {
    analogWrite(pin_pwm_l, 0);
    analogWrite(pin_pwm_r, currentMotorVoltage);
  } else if (currentMotorVoltage < 0) {
    analogWrite(pin_pwm_l, -currentMotorVoltage);
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
  return true;
}

boolean DomeMotor::isParked() {
  return false;
}

boolean DomeMotor::isHome() {
  return false;
}

float DomeMotor::getHomePositionInDegrees() {
  return 0;
}

long DomeMotor::getHomePositionInSteps() {
  return 0;
}

void DomeMotor::syncCurrentPositionAsDegrees(float degrees) {

}

void DomeMotor::setPark(float degrees) {

}

void DomeMotor::setPark(long steps) {

}

void DomeMotor::setPark() {

}

void DomeMotor::slewTo(float degrees) {

}

void DomeMotor::slewTo(long steps) {

}

void DomeMotor::slewToHome() {

}

void DomeMotor::slewToPark() {

}

void DomeMotor::slewRight() {
  _motorVoltageTarget = 255;
}

void DomeMotor::slewLeft() {
  _motorVoltageTarget = -255;
}

void DomeMotor::stop() {
  _motorVoltageTarget = 0;
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

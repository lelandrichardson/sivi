#include "Motor.h"



Motor::Motor(uint8_t rightPin, uint8_t leftPin) {

}

boolean Motor::begin() {
  return true;
}

void Motor::loop() {

}

int Motor::position() {
  return 0;
}

int Motor::min() {
  return 0;
}

int Motor::max() {
  return 10;
}

boolean Motor::isMoving() {
  return false;
}

void Motor::setRange(int min, int max) {

}

void Motor::goTo(int position) {

}

void Motor::stop() {

}

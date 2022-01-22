#include "Arduino.h"
#include "Motor.h"

#define REV_PER_MIN 60

Motor::Motor(
  int steps_per_revolution,
  int motor_pin_1, 
  int motor_pin_2,
  int motor_pin_3, 
  int motor_pin_4)
{
  this->current = 0;
  this->last_step_time = 0;
  this->steps_per_revolution = steps_per_revolution;
  this->min = -steps_per_revolution;
  this->max = steps_per_revolution;
  this->step_delay = 60L * 1000L * 1000L / this->steps_per_revolution / REV_PER_MIN;

  // Arduino pins for the motor control connection:
  this->motor_pin_1 = motor_pin_1;
  this->motor_pin_2 = motor_pin_2;
  this->motor_pin_3 = motor_pin_3;
  this->motor_pin_4 = motor_pin_4;

  // setup the pins on the microcontroller:
  pinMode(this->motor_pin_1, OUTPUT);
  pinMode(this->motor_pin_2, OUTPUT);
  pinMode(this->motor_pin_3, OUTPUT);
  pinMode(this->motor_pin_4, OUTPUT);
}

void Motor::stop()
{
  this->target = this->current;
}

boolean Motor::isMoving()
{
  return this->target != this->current;
}

int Motor::position()
{
  return this->current;
}
int Motor::getMin()
{
  return this->min;
}
int Motor::getMax()
{
  return this->max;
}

/*
 * Moves the motor steps_to_move steps.  If the number is negative,
 * the motor moves in the reverse direction.
 */
void Motor::goTo(int position)
{
  if (this->min > position) {
    position = this->min;
  }
  if (this->max < position) {
    position = this->max;
  }
  this->target = position;
}

void Motor::setRange(int min, int max) {
  if (min >= max) return;
  this->min = min;
  this->max = max;
  this->goTo(this->target);
}

void Motor::setMin(int min) {
  if (min >= this->max) return;
  this->min = min;
  this->goTo(this->target);
}

void Motor::setMax(int max) {
  if (this->min >= max) return;
  this->max = max;
  this->goTo(this->target);
}

void Motor::loop()
{
  if (this->target == this->current) return;

  unsigned long now = micros();

  // move only if the appropriate delay has passed:
  if (now - this->last_step_time >= this->step_delay)
  {
    // get the timeStamp of when you stepped:
    this->last_step_time = now;
    // increment or decrement the step number,
    // depending on direction:
    if (this->target > this->current)
    {
      this->current++;
    }
    else
    {
      this->current--;
    }
    // step the motor to step number 0, 1, ..., {3 or 10}
    stepMotor(this->current % 4);
  }
}

/*
 * Moves the motor forward or backwards.
 */
void Motor::stepMotor(int thisStep)
{
  switch (thisStep) {
    case 0:  // 1010
      digitalWrite(motor_pin_1, HIGH);
      digitalWrite(motor_pin_2, LOW);
      digitalWrite(motor_pin_3, HIGH);
      digitalWrite(motor_pin_4, LOW);
    break;
    case 1:  // 0110
      digitalWrite(motor_pin_1, LOW);
      digitalWrite(motor_pin_2, HIGH);
      digitalWrite(motor_pin_3, HIGH);
      digitalWrite(motor_pin_4, LOW);
    break;
    case 2:  //0101
      digitalWrite(motor_pin_1, LOW);
      digitalWrite(motor_pin_2, HIGH);
      digitalWrite(motor_pin_3, LOW);
      digitalWrite(motor_pin_4, HIGH);
    break;
    case 3:  //1001
      digitalWrite(motor_pin_1, HIGH);
      digitalWrite(motor_pin_2, LOW);
      digitalWrite(motor_pin_3, LOW);
      digitalWrite(motor_pin_4, HIGH);
    break;
  }
}
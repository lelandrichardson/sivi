#include <Stepper.h>

// change this to the number of steps on your motor
#define STEPS 200

// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to
Stepper stepper(STEPS, 25, 26, 27, 15);


void setup()
{
  Serial.begin(9600);
  Serial.println("Stepper test!");
  stepper.setSpeed(30);
}

void loop()
{
  // Serial.println("Forward");
  // stepper.step(STEPS);
  // delay(1000);
  // Serial.println("Backward");
  // stepper.step(-STEPS);
  // delay(1000);
}
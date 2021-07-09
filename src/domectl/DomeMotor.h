#include <Arduino.h>
#include <analogWrite.h>
#include "../shared/Constants.h"

#define ENCODER_STEPS_PER_ROTATION 2400
#define WHEEL_DIAMETER_INCHES 2.975
#define DOME_DIAMETER_INCHES 120.2

class DomeMotor {
  public:
    DomeMotor(uint8_t rightPin, uint8_t leftPin) : pin_pwm_r(rightPin), pin_pwm_l(leftPin) {}
    boolean begin(boolean A, boolean B);
    void encoderPulse(boolean A, boolean B);
    void loop();
    long getPositionInSteps();
    float getPositionInDegrees();
    float getVelocityInDegreesPerSec();
    float getVelocityInStepsPerSec();
    boolean isSlewing();
    boolean isParked();
    boolean isHome();
    float getHomePositionInDegrees();
    long getHomePositionInSteps();
    void syncCurrentPositionAsDegrees(float degrees);
    void setPark(float degrees);
    void setPark(long steps);
    void setPark();
    void slewTo(float degrees);
    void slewTo(long steps);
    void slewToHome();
    void slewToPark();
    void slewRight();
    void slewLeft();
    void stop();

  private:
    uint8_t stateOf(boolean A, boolean B);

    volatile long encoderPosition = 0;
    volatile uint8_t encoderState = 0;
    unsigned long lastVelocityTime;
    long lastVelocityPosition;
    // velocity in encoder steps per microsecond
    float currentVelocity = 0.0;

    // It's unclear to me if we should just figure out what this number is and make it a constant, or have the software update this as it
    // goes. It's probably not worth updating it since the dome will rarely undergo multiple rotations in the course of a night.
    static const unsigned long stepsPerDomeRotation = ENCODER_STEPS_PER_ROTATION * DOME_DIAMETER_INCHES / WHEEL_DIAMETER_INCHES; // PI cancels out

    volatile int _motorVoltageTarget = 0;
    int currentMotorVoltage = 0;
    uint8_t pin_pwm_r;
    uint8_t pin_pwm_l;
};
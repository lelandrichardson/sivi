#include <Arduino.h>
#include <analogWrite.h>
#include <PID_v1.h>
#include "src/Constants.h"


class DomeMotor {
  public:
    DomeMotor(uint8_t rightPin, uint8_t leftPin);
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
    long degreesToSteps(float degrees);
    float stepsToDegrees(long steps);
    long nearestEquivalentSteps(long steps);
    long equivalentSteps(long steps, long relativeTo, long mod);
    void manualMode();
    void automaticMode();
    boolean isEffectivelyAtPosition(long steps);

    void setMotorSpeed(double speed);

    boolean automatic;

    double input;
    double output;
    double setPoint;

    double kp;
    double ki;
    double kd;
    PID pid;

    long parkPositionInSteps;
    long homePositionInSteps;

    volatile long encoderPosition = 0;
    volatile uint8_t encoderState = 0;
    unsigned long lastVelocityTime;
    long lastVelocityPosition;
    // velocity in encoder steps per microsecond
    float currentVelocity = 0.0;

    unsigned long stepsPerDomeRotation;
    unsigned long minimumErrorDistance;
    // TODO: is this equivalent to homePositionInSteps???
    long stepsToTrueNorthOffset = 0;

    volatile int _motorVoltageTarget = 0;
    int currentMotorVoltage = 0;
    uint8_t pin_pwm_r;
    uint8_t pin_pwm_l;
};
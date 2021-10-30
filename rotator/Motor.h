#include <Arduino.h>
#include "src/Constants.h"

class Motor {
  public:
    Motor(uint8_t rightPin, uint8_t leftPin);
    boolean begin();
    void loop();

    int position();
    int min();
    int max(); 
    boolean isMoving();

    void setRange(int min, int max);
    void goTo(int position);
    void stop();
    void loop();
};
#include <Arduino.h>
#include <functional>
#if defined(WIFI_Kit_32)
#include "heltec.h"

typedef std::function<void(void)> TDrawFunction;

typedef enum Align {
  Left,
  Center,
  Right
};

class Display {
public:
  boolean begin();
  void loop();
  void wake();
  void toggle();
  boolean isSleeping();
  void toast(String msg);
  void onDraw(TDrawFunction draw);

  void text(Align alignment, int16_t alignX, String msg);
  void nextLine();

protected:
  SSD1306Wire *heltec;
  TDrawFunction drawHandler;
  int16_t currentLine;
  unsigned long displayLastRefreshTime;
  volatile boolean displayIsSleeping = false;
  volatile unsigned long displayWakeTime;
  unsigned long displayToastMessageShown;
  String displayToastMessage;
};
#endif
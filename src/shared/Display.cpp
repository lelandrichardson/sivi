#include "Display.h"
#include "Constants.h"

const unsigned long MICROS_UNTIL_DISPLAY_SLEEP = 30 * MICROS_PER_SEC;

boolean Display::begin() {
  Heltec.begin(
    true /*DisplayEnable Enable*/, 
    false /*LoRa Enable*/, 
    false /*Serial Enable*/ // NOTE: Serial connection is enabled elsewhere
  );
  heltec = Heltec.display;
  displayLastRefreshTime = micros();
  displayWakeTime = micros();
  return true;
}

void Display::wake() {
  displayIsSleeping = false;
  displayWakeTime = micros();
}

void Display::toggle() {
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

boolean Display::isSleeping() {
  return displayIsSleeping;
}

void Display::toast(String msg) {
  displayToastMessage = msg;
  displayToastMessageShown = micros();
}

void Display::onDraw(TDrawFunction draw) {
  drawHandler = draw;
}

void Display::text(Align alignment, int16_t alignX, String msg) {
  const int LINE_HEIGHT = 10;
  int16_t msgWidth;
  int16_t startX;
  switch (alignment) {
    case Left:
      heltec -> drawString(alignX, currentLine * LINE_HEIGHT, msg);
      break;
    case Center:
      msgWidth = heltec -> getStringWidth(msg);
      startX = max(0, alignX - (msgWidth / 2));
      heltec -> drawString(startX, currentLine * LINE_HEIGHT, msg);
      break;
    case Right:
      msgWidth = heltec -> getStringWidth(msg);
      startX = max(0, alignX - msgWidth);
      heltec->drawString(startX, currentLine * LINE_HEIGHT, msg);
      break;
  }
}
void Display::nextLine() {
  currentLine++;
}

void Display::loop() {
  if (displayIsSleeping) return;

  unsigned long currentTime = micros();
  unsigned long microsSinceWake = currentTime - displayWakeTime;
  unsigned long timeSinceLastRefresh = currentTime - displayLastRefreshTime;

  if (microsSinceWake > MICROS_UNTIL_DISPLAY_SLEEP) {
    displayIsSleeping = true;
    heltec -> clear();
    heltec -> display();
    return;
  }


  if (timeSinceLastRefresh > 100 * MICROS_PER_MS) {
    displayLastRefreshTime = currentTime;

    // update display 2 times / second
    heltec -> clear();

    currentLine = 0;

    // display will go to sleep after 30s. 
    // Display time left in the top right
    int secondsUntilSleep = (MICROS_UNTIL_DISPLAY_SLEEP - microsSinceWake) / MICROS_PER_SEC;
    char buf2[8];
    dtostrf(secondsUntilSleep, 3, 0, buf2);
    text(Right, DISPLAY_WIDTH, buf2);

    drawHandler();

    unsigned long timeSinceToastMessage = currentTime - displayToastMessageShown;

    if (timeSinceToastMessage < 5 * MICROS_PER_SEC) {
      currentLine = 5;
      // Debug messaging. Message active for 5 seconds.
      int toastSecsLeft = (5 * MICROS_PER_SEC - timeSinceToastMessage) / MICROS_PER_SEC;
      text(Left, 0, "-");
      char buf[8];
      sprintf(buf, "%d -", toastSecsLeft);
      text(Right, DISPLAY_WIDTH, buf);
      text(Center, DISPLAY_WIDTH / 2, "GETAZIMUTH");
    }


    heltec -> display();
  }
}
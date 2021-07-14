#include "SerialCommander.h"



SerialCommander::SerialCommander() {

}

SerialCommander::~SerialCommander() {
  RequestHandler* handler = _firstHandler;
  while (handler) {
    RequestHandler* next = handler->next();
    delete handler;
    handler = next;
  }
}

boolean SerialCommander::begin() {
  return true;
}

void SerialCommander::loop() {
  serialEvent();
  if (pendingSerialCommand) {
    parseCommand();
    String cmd = String(serialCommand);
    RequestHandler* handler = _firstHandler;
    boolean handled = false;
    while (handler) {
      RequestHandler* next = handler->next();
      if (cmd.equals(handler->command)) {
        handled = true;
        handler->handle();
        break;
      }
      handler = next;
    }
    if (!handled) {
      _notFoundHandler();
    }
  }
}
void SerialCommander::on(const String &cmd, THandlerFunction handler) {
  addRequestHandler(new RequestHandler(cmd, handler));
}
void SerialCommander::onNotFound(THandlerFunction handler) {
  _notFoundHandler = handler;
}
void SerialCommander::addRequestHandler(RequestHandler* handler) {
  if (!_lastHandler) {
    _firstHandler = handler;
    _lastHandler = handler;
  }
  else {
    _lastHandler->next(handler);
    _lastHandler = handler;
  }
}
char * SerialCommander::argString(int i) {
  char * arg;
  char * source = serialArguments;
  while (i --> 0) {
    arg = strtok(source, SERIAL_ARG_DELIMITER);
    source = NULL;
  }
  return arg;
}
int SerialCommander::argInt(int i) {
  return atoi(argString(i));
}
float SerialCommander::argFloat(int i) {
  return atof(argString(i));
}
void SerialCommander::response(int code) {
  response(code, "");
}
void SerialCommander::response(int code, String result) {
  serialResponse(String(serialCommand), code, result);
}

void SerialCommander::success(String result) {
  serialResponse(String(serialCommand), 200, result);
}

void SerialCommander::success(boolean result) {
  success(String(result));
}

void SerialCommander::success(float result, int precision) {
  success(String(result, precision));
}

void SerialCommander::success(long result) {
  success(String(result));
}
void SerialCommander::success() {
  success("1");
}


// read serial data and store everything inside of start/end inside of the "serialUnparsed"
// string.
void SerialCommander::serialEvent() {
  static boolean started = false;
  static int i = 0;
  char next;

  while (Serial.available() > 0 && pendingSerialCommand == false) {
    next = Serial.read();

    if (started == true) {
      if (next != SERIAL_END) {
        serialUnparsed[i] = next;
        i++;
        if (i >= SERIAL_COMMAND_MAX_SIZE) {
            i = SERIAL_COMMAND_MAX_SIZE - 1;
        }
      }
      else {
        serialUnparsed[i] = '\0';
        started = false;
        i = 0;
        pendingSerialCommand = true;
      }
    } else if (next == SERIAL_START) {
      started = true;
    }
  }
}

// parse the "serialUnparsed" string and store into serialCommand and serialArguments. 
// It should be of the form:
// COMMAND ':' ARGS
void SerialCommander::parseCommand() {
  char * strtokIndx;
  strtokIndx = strtok(serialUnparsed, SERIAL_DELIMITER);
  strcpy(serialCommand, strtokIndx);
  strtokIndx = strtok(NULL, SERIAL_DELIMITER);
  // the 2nd part might be null if no arguments were supplied
  if (strtokIndx != NULL) {
    strcpy(serialArguments, strtokIndx);
  } else {
    serialArguments[0] = '\0';
  }
  pendingSerialCommand = false;
}

void SerialCommander::serialResponse(String cmd, int code, String result) {
  Serial.print(SERIAL_START);
  Serial.print(cmd);
  Serial.print(SERIAL_DELIMITER);
  Serial.print(code);
  Serial.print(SERIAL_DELIMITER);
  Serial.print(result);
  Serial.println(SERIAL_END);
}

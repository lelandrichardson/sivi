#include "AscomApi.h"



AscomApi::AscomApi() {

}

AscomApi::~AscomApi() {
  AscomHandler* handler = _firstHandler;
  while (handler) {
    AscomHandler* next = handler->next();
    delete handler;
    handler = next;
  }
}

boolean AscomApi::begin() {
  return true;
}
boolean AscomApi::begin(char *ssid, char *password) {
  boolean success = begin();
  #ifdef WIFI_Kit_8

  // start the web server
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  server = new ESP8266WebServer(80);
  server->onNotFound([this]() { this->handleWebNotFound(); });
  server->begin();
  #endif

  return success;
}

void AscomApi::loop() {
  serialEvent();
  if (pendingSerialRequest) {
    parseRequest();
    String request = String(serialRequest);
    AscomHandler* handler = _firstHandler;
    boolean handled = false;
    while (handler) {
      AscomHandler* next = handler->next();
      if (request.equals(handler->name)) {
        handled = true;
        handler->handle(this);
        break;
      }
      handler = next;
    }
    if (!handled) {
      _notFoundHandler();
    }
  }
  #ifdef WIFI_Kit_8
  server->handleClient();
  #endif
}

void AscomApi::handleWebNotFound() {

}


void AscomApi::on(const String &cmd, AscomMethod handler) {
  addRequestHandler(new AscomCommandHandler(cmd, handler));
}

void AscomApi::propertyFloat(const String &name, AscomFloatGetter getter) {

}
void AscomApi::propertyFloat(const String &name, AscomFloatGetter getter, AscomFloatSetter setter) {

}

void AscomApi::propertyInt(const String &name, AscomIntGetter getter) {

}
void AscomApi::propertyInt(const String &name, AscomIntGetter getter, AscomIntSetter setter) {

}

void AscomApi::propertyString(const String &name, AscomStringGetter getter) {

}
void AscomApi::propertyString(const String &name, AscomStringGetter getter, AscomStringSetter setter) {

}

void AscomApi::command(const String &cmd, AscomMethod method) {

}

void AscomApi::onNotFound(AscomMethod handler) {
  _notFoundHandler = handler;
}
void AscomApi::addRequestHandler(AscomHandler* handler) {
  if (!_lastHandler) {
    _firstHandler = handler;
    _lastHandler = handler;
  }
  else {
    _lastHandler->next(handler);
    _lastHandler = handler;
  }
}
boolean AscomApi::hasArgs() {
  return false;
}
char * AscomApi::argString(int i) {
  char * arg;
  char * source = serialArguments;
  while (i --> 0) {
    arg = strtok(source, SERIAL_ARG_DELIMITER);
    source = NULL;
  }
  return arg;
}
int AscomApi::argInt(int i) {
  return atoi(argString(i));
}
float AscomApi::argFloat(int i) {
  return atof(argString(i));
}
void AscomApi::response(int code) {
  response(code, "");
}
void AscomApi::response(int code, String result) {
  serialResponse(String(serialRequest), code, result);
}

void AscomApi::success(String result) {
  serialResponse(String(serialRequest), 200, result);
}

void AscomApi::success(boolean result) {
  success(String(result));
}

void AscomApi::success(float result, int precision) {
  success(String(result, precision));
}

void AscomApi::success(long result) {
  success(String(result));
}
void AscomApi::success() {
  success("1");
}


// read serial data and store everything inside of start/end inside of the "serialUnparsed"
// string.
void AscomApi::serialEvent() {
  static boolean started = false;
  static int i = 0;
  char next;

  while (Serial.available() > 0 && pendingSerialRequest == false) {
    next = Serial.read();

    if (started == true) {
      if (next != SERIAL_END) {
        serialUnparsed[i] = next;
        i++;
        if (i >= SERIAL_REQUEST_MAX_SIZE) {
            i = SERIAL_REQUEST_MAX_SIZE - 1;
        }
      }
      else {
        serialUnparsed[i] = '\0';
        started = false;
        i = 0;
        pendingSerialRequest = true;
      }
    } else if (next == SERIAL_START) {
      started = true;
    }
  }
}

// parse the "serialUnparsed" string and store into serialCommand and serialArguments. 
// It should be of the form:
// COMMAND ':' ARGS
void AscomApi::parseRequest() {
  char * strtokIndx;
  strtokIndx = strtok(serialUnparsed, SERIAL_DELIMITER);
  strcpy(serialRequest, strtokIndx);
  strtokIndx = strtok(NULL, SERIAL_DELIMITER);
  // the 2nd part might be null if no arguments were supplied
  if (strtokIndx != NULL) {
    strcpy(serialArguments, strtokIndx);
  } else {
    serialArguments[0] = '\0';
  }
  pendingSerialRequest = false;
}

void AscomApi::serialResponse(String cmd, int code, String result) {
  Serial.print(SERIAL_START);
  Serial.print(cmd);
  Serial.print(SERIAL_DELIMITER);
  Serial.print(code);
  Serial.print(SERIAL_DELIMITER);
  Serial.print(result);
  Serial.println(SERIAL_END);
}

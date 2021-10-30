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
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  server = new WiFiServer(80);
  server->begin();
  return success;
}

void AscomApi::loop() {
  serialEvent();
  webEvent();
}

void AscomApi::handleRequest() {
  String request = String(requestTarget);
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

void AscomApi::on(const String &cmd, AscomMethod handler) {
  addRequestHandler(new AscomCommandHandler(cmd, handler));
}

void AscomApi::propertyFloat(const String &name, AscomFloatGetter getter) {
  addRequestHandler(new AscomFloatPropertyHandler(name, getter));
}

void AscomApi::propertyFloat(const String &name, AscomFloatGetter getter, AscomFloatSetter setter) {
  addRequestHandler(new AscomFloatPropertyHandler(name, getter, setter));
}

void AscomApi::propertyInt(const String &name, AscomIntGetter getter) {
  addRequestHandler(new AscomIntPropertyHandler(name, getter));
}

void AscomApi::propertyInt(const String &name, AscomIntGetter getter, AscomIntSetter setter) {
  addRequestHandler(new AscomIntPropertyHandler(name, getter, setter));
}

void AscomApi::propertyString(const String &name, AscomStringGetter getter) {
  addRequestHandler(new AscomStringPropertyHandler(name, getter));
}

void AscomApi::propertyString(const String &name, AscomStringGetter getter, AscomStringSetter setter) {
  addRequestHandler(new AscomStringPropertyHandler(name, getter, setter));
}

void AscomApi::command(const String &cmd, AscomMethod method) {
  addRequestHandler(new AscomCommandHandler(cmd, method));
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
  char * source = requestArguments;
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
  serialResponse(String(requestTarget), code, result);
}

void AscomApi::success(String result) {
  serialResponse(String(requestTarget), 200, result);
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

// read serial data and store everything inside of start/end inside of the "unparsedRequest"
// string.
void AscomApi::serialEvent() {
  if (webRequestStarted) return;
  static boolean started = false;
  static int i = 0;
  char next;

  while (Serial.available() > 0) {
    next = Serial.read();

    if (serialRequestStarted == true) {
      if (next != SERIAL_END) {
        unparsedRequest[i] = next;
        i++;
        if (i >= REQUEST_MAX_SIZE) {
            i = REQUEST_MAX_SIZE - 1;
        }
      }
      else {
        unparsedRequest[i] = '\0';
        serialRequestStarted = false;
        i = 0;
        requestFulfilled = false;
        parseSerialRequest();
        handleRequest();
        break;
      }
    } else if (next == SERIAL_START) {
      serialRequestStarted = true;
    }
  }
}

void AscomApi::webEvent() {
  if (serialRequestStarted) return;
  static int i = 0;
  client = server->available();
  if (client) {
    webRequestStarted = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (i == 0) {
            unparsedRequest[i] = '\0';
            i = 0;
            webRequestStarted = false;
            requestFulfilled = false;
            // at this point we have read the full request, so we parse it and handle it here.
            // break out of the while loop:
            parseWebRequest();
            handleRequest();
            break;
          } else {    // if you got a newline, then clear currentLine:
            i = 0;
          }
        } else if (c != '\r') {
          // if you got anything else but a carriage return character,
          // add it to the end of the currentLine
          unparsedRequest[i] = c;
          i++;
          if (i >= REQUEST_MAX_SIZE) {
            i = REQUEST_MAX_SIZE - 1;
          }
        }
      }
    }
    // close the connection:
    client.stop();
    webRequestStarted = false;
  }
}

// parse the "unparsedRequest" string and store into requestTarget and requestArguments. 
// It should be of the form:
// COMMAND ':' ARGS
void AscomApi::parseWebRequest() {
  char * strtokIndx;
  char * method;
  strtokIndx = strtok(unparsedRequest, " ");
  String methodStr = String(method);
  if (methodStr.equals("POST")) {
    requestType = post;
  } else {
    // default to GET
    requestType = get;
  }
  strcpy(method, strtokIndx);
  strtokIndx = strtok(NULL, "/"); // this should be the very next character
  strtokIndx = strtok(NULL, "?");
  strcpy(requestTarget, strtokIndx);
  strtokIndx = strtok(NULL, "?");
  // the 2nd part might be null if no arguments were supplied
  if (strtokIndx != NULL) {
    strcpy(requestArguments, strtokIndx);
  } else {
    requestArguments[0] = '\0';
  }
  requestProtocol = http;
}

// parse the "unparsedRequest" string and store into requestTarget and requestArguments. 
// It should be of the form:
// METHOD ' ' '/' TARGET '?' ARGUMENTS
void AscomApi::parseSerialRequest() {
  char * strtokIndx;
  strtokIndx = strtok(unparsedRequest, SERIAL_DELIMITER);
  strcpy(requestTarget, strtokIndx);
  strtokIndx = strtok(NULL, SERIAL_DELIMITER);
  // the 2nd part might be null if no arguments were supplied
  if (strtokIndx != NULL) {
    strcpy(requestArguments, strtokIndx);
  } else {
    requestArguments[0] = '\0';
  }
  requestProtocol = serial;
}

void AscomApi::serialResponse(String cmd, int code, String result) {
  if (requestFulfilled) return;
  Serial.print(SERIAL_START);
  Serial.print(cmd);
  Serial.print(SERIAL_DELIMITER);
  Serial.print(code);
  Serial.print(SERIAL_DELIMITER);
  Serial.print(result);
  Serial.println(SERIAL_END);
  requestFulfilled = true;
}

// Lots of HTTP status codes are in circulation, but we will only be using a few.
String AscomApi::responseCodeToString(const int code) {
  switch (code) {
    // case 100: return F("Continue");
    // case 101: return F("Switching Protocols");
    case 200: return F("OK");
    // case 201: return F("Created");
    // case 202: return F("Accepted");
    // case 203: return F("Non-Authoritative Information");
    // case 204: return F("No Content");
    // case 205: return F("Reset Content");
    // case 206: return F("Partial Content");
    // case 300: return F("Multiple Choices");
    // case 301: return F("Moved Permanently");
    // case 302: return F("Found");
    // case 303: return F("See Other");
    // case 304: return F("Not Modified");
    // case 305: return F("Use Proxy");
    // case 307: return F("Temporary Redirect");
    case 400: return F("Bad Request");
    // case 401: return F("Unauthorized");
    // case 402: return F("Payment Required");
    // case 403: return F("Forbidden");
    case 404: return F("Not Found");
    // case 405: return F("Method Not Allowed");
    // case 406: return F("Not Acceptable");
    // case 407: return F("Proxy Authentication Required");
    // case 408: return F("Request Time-out");
    // case 409: return F("Conflict");
    // case 410: return F("Gone");
    // case 411: return F("Length Required");
    // case 412: return F("Precondition Failed");
    // case 413: return F("Request Entity Too Large");
    // case 414: return F("Request-URI Too Large");
    // case 415: return F("Unsupported Media Type");
    // case 416: return F("Requested range not satisfiable");
    // case 417: return F("Expectation Failed");
    // case 418: return F("I'm a teapot");
    case 500: return F("Internal Server Error");
    // case 501: return F("Not Implemented");
    // case 502: return F("Bad Gateway");
    // case 503: return F("Service Unavailable");
    // case 504: return F("Gateway Time-out");
    // case 505: return F("HTTP Version not supported");
    default:  return F("");
  }
}

void AscomApi::webResponse(String cmd, int code, String result) {
  if (requestFulfilled) return;
  String codeString = responseCodeToString(code);
  client.print("HTTP/1.1 ");
  client.print(code);
  client.print(" ");
  client.println(codeString);
  // TODO: toggle between text/json?
  client.println("Content-Type: plain/text");
  client.println("Connection: close");
  client.println();
  client.println(result);
  requestFulfilled = true;
}

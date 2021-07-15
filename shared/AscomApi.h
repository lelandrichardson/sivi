#include "Arduino.h"
#include <functional>
#include "AscomRequestHandlers.h"

#ifndef ASCOM_API_H
#define ASCOM_API_H

#ifdef WIFI_Kit_8
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif

#ifdef WIFI_Kit_32
#include <Wifi.h>
#endif

#ifndef SERIAL_CONSTANTS
#define SERIAL_CONSTANTS
#define SERIAL_START '<'
#define SERIAL_END '>'
#define SERIAL_DELIMITER ":"
#define SERIAL_ARG_DELIMITER ","
#endif

// ascom.begin("name", "ssid", "password")
// ascom.propertyFloat(
//   "WindSpeed",
//   []() { return foo.windSpeed(); },
// );
// ascom.propertyInt(
//   "Position",
//   []() { return foo.position(); },
//   [](value) { foo.updatePosition(value); }
// );
// ascom.command(
//   "slewTo",
//   []() { 
//     foo.slewTo(ascom.argFloat(0));
//     ascom.success();
//   },
// );
// ascom.settingsFloat(
//   "ki",
//   []() { return foo.ki; },
//   [](value) { foo.ki = value; }
// );


class AscomApi {
public:
  AscomApi();
  ~AscomApi();
  boolean begin();
  boolean begin(char *ssid, char *password);
  void loop();

  void propertyFloat(const String &name, AscomFloatGetter getter);
  void propertyFloat(const String &name, AscomFloatGetter getter, AscomFloatSetter setter);

  void propertyInt(const String &name, AscomIntGetter getter);
  void propertyInt(const String &name, AscomIntGetter getter, AscomIntSetter setter);

  void propertyString(const String &name, AscomStringGetter getter);
  void propertyString(const String &name, AscomStringGetter getter, AscomStringSetter setter);

  void command(const String &cmd, AscomMethod method);
  
  void on(const String &cmd, AscomMethod handler);
  void onNotFound(AscomMethod handler);
  
  char * argString(int i);
  int argInt(int i);
  float argFloat(int i);
  boolean hasArgs();
  void response(int code);
  void response(int code, String result);
  void success(String result);
  void success(boolean result);
  void success(float result, int precision);
  void success(long result);
  void success();

protected:
  static const int SERIAL_REQUEST_MAX_SIZE = 200;
  void serialEvent();
  void parseRequest();
  void serialResponse(String cmd, int code, String result);
  void addRequestHandler(AscomHandler* handler);
  void handleWebNotFound();

#ifdef WIFI_Kit_8
  ESP8266WebServer *server = nullptr;
#endif


  AscomHandler* _firstHandler;
  AscomHandler* _lastHandler;
  AscomMethod _notFoundHandler;
  char serialUnparsed[SERIAL_REQUEST_MAX_SIZE];
  char serialRequest[SERIAL_REQUEST_MAX_SIZE];
  char serialArguments[SERIAL_REQUEST_MAX_SIZE];
  boolean pendingSerialRequest;
};

#endif
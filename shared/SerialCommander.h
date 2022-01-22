#include "Arduino.h"
#include <functional>

#ifndef SERIAL_CONSTANTS
#define SERIAL_CONSTANTS
#define SERIAL_START '<'
#define SERIAL_END '>'
#define SERIAL_DELIMITER ":"
#define SERIAL_ARG_DELIMITER ","
#endif

typedef std::function<void(void)> THandlerFunction;

class RequestHandler {
public:
    RequestHandler(const String &cmd, THandlerFunction fn) : _fn(fn), command(cmd) {};
    ~RequestHandler() {}
    RequestHandler* next() { return _next; }
    void next(RequestHandler* r) { _next = r; }
    void handle() { _fn(); }
    String command;
private:
    RequestHandler* _next = nullptr;
    THandlerFunction _fn;
};

class SerialCommander {
public:
  SerialCommander();
  ~SerialCommander();
  boolean begin();
  void loop();
  
  void on(const String &cmd, THandlerFunction handler);
  void onNotFound(THandlerFunction handler);
  
  char * argString(int i);
  int argInt(int i);
  float argFloat(int i);
  void response(int code);
  void response(int code, String result);
  void success(String result);
  void success(int result);
  void success(float result, int precision);
  void success(long result);
  void success();

protected:
  static const int SERIAL_COMMAND_MAX_SIZE = 200;
  void serialEvent();
  void parseCommand();
  void serialResponse(String cmd, int code, String result);
  void addRequestHandler(RequestHandler* handler);

  RequestHandler*  _firstHandler;
  RequestHandler*  _lastHandler;
  THandlerFunction _notFoundHandler;
  char serialUnparsed[SERIAL_COMMAND_MAX_SIZE];
  char serialCommand[SERIAL_COMMAND_MAX_SIZE];
  char serialArguments[SERIAL_COMMAND_MAX_SIZE];
  boolean pendingSerialCommand;
};

#include <Arduino.h>
#include <functional>

#ifndef ASCOM_REQUEST_HANDLERS_H
#define ASCOM_REQUEST_HANDLERS_H

typedef std::function<void(void)> AscomMethod;

typedef std::function<float(void)> AscomFloatGetter;
typedef std::function<void(float)> AscomFloatSetter;

typedef std::function<int(void)> AscomIntGetter;
typedef std::function<void(int)> AscomIntSetter;

typedef std::function<char *(void)> AscomStringGetter;
typedef std::function<void(char *)> AscomStringSetter;

class AscomApi;
class AscomHandler {
public:
  AscomHandler(const String &name) : name(name) {};
  AscomHandler* next() { return nullptr; }
  void next(AscomHandler* r) {  }
  virtual void handle(AscomApi *api);
  String name;
private:
  // AscomHandler* _next = nullptr;
};

class AscomCommandHandler : public AscomHandler {
public:
  AscomCommandHandler(
    const String &name, 
    AscomMethod fn) : AscomHandler(name), _fn(fn) {};
  void handle(AscomApi *api);
private:
  AscomMethod _fn;
};

class AscomFloatPropertyHandler : public AscomHandler {
public:
  AscomFloatPropertyHandler(
    const String &name, 
    AscomFloatGetter getter, 
    AscomFloatSetter setter) : AscomHandler(name), _getter(getter), _setter(setter) {};
  AscomFloatPropertyHandler(const String &name, AscomFloatGetter getter);
  void handle(AscomApi *api);
private:
  AscomFloatGetter _getter;
  AscomFloatSetter _setter;
};

class AscomIntPropertyHandler : public AscomHandler {
public:
  AscomIntPropertyHandler(
    const String &name, 
    AscomIntGetter getter, 
    AscomIntSetter setter) : AscomHandler(name), _getter(getter), _setter(setter) {};
  AscomIntPropertyHandler(const String &name, AscomIntGetter getter);
  void handle(AscomApi *api);
private:
  AscomIntGetter _getter;
  AscomIntSetter _setter;
};

class AscomStringPropertyHandler : public AscomHandler {
public:
  AscomStringPropertyHandler(
    const String &name, 
    AscomStringGetter getter, 
    AscomStringSetter setter) : AscomHandler(name), _getter(getter), _setter(setter) {};
  AscomStringPropertyHandler(const String &name, AscomStringGetter getter);
  void handle(AscomApi *api);
private:
  AscomStringGetter _getter;
  AscomStringSetter _setter;
};

#endif
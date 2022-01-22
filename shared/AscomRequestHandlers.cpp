#include "AscomRequestHandlers.h"
#include "AscomApi.h"

void emptyFloatSetter(float value) {}
void emptyStringSetter(char *value) {}
void emptyIntSetter(int value) {}

AscomFloatPropertyHandler::AscomFloatPropertyHandler(
  const String &name, 
  AscomFloatGetter getter
) : AscomHandler(name), _getter(getter), _setter(emptyFloatSetter) {};

AscomIntPropertyHandler::AscomIntPropertyHandler(
  const String &name, 
  AscomIntGetter getter
) : AscomHandler(name), _getter(getter), _setter(emptyIntSetter) {};

AscomStringPropertyHandler::AscomStringPropertyHandler(
  const String &name, 
  AscomStringGetter getter
) : AscomHandler(name), _getter(getter), _setter(emptyStringSetter) {};


void AscomFloatPropertyHandler::handle(AscomApi *api) {
  if (api->hasArgs()) {
    api->success(_getter(), 8);
  } else {
    _setter(api->argFloat(1));
    api->success();
  }
}

void AscomIntPropertyHandler::handle(AscomApi *api) {
  if (api->hasArgs()) {
    api->success(_getter());
  } else {
    _setter(api->argFloat(1));
    api->success();
  }
}

void AscomStringPropertyHandler::handle(AscomApi *api) {
  if (api->hasArgs()) {
    _setter(api->argString(1));
    api->success();
  } else {
    api->success(_getter());
  }
}

void AscomCommandHandler::handle(AscomApi *api) {
  // NOTE: we could prevent the method from getting executed
  // if the request is a web GET request. This might be a good
  // idea, but for convenience I am going to run it regardless.
  _fn();
  api->success();
}
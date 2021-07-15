#include "AscomRequestHandlers.h"
#include "AscomApi.h"

void AscomFloatPropertyHandler::handle(AscomApi *api) {
  if (api->hasArgs()) {
    _setter(api->argFloat(1));
    api->success();
  } else {
    api->success(_getter(), 8);
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
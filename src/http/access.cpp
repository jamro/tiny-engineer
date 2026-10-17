#include "http/access.h"

#include "http/response.h"
#include "network/wifi_connect.h"
#include "settings/settings.h"

namespace {

bool tokenAccepted(WebServer& server) {
  return !settingsAccessTokenSet() ||
         server.header("Authorization").equalsConstantTime(String("Bearer ") + settingsAccessToken());
}

}  // namespace

std::optional<Refusal> Refusal::check(WebServer& server, Access access) {
  if (access == Access::Configured && !settingsProvisioned()) {
    return Refusal{503, "not provisioned"};
  }

  if (access != Access::Public && !tokenAccepted(server)) {
    return Refusal{401, "unauthorized"};
  }

  if (access == Access::Setup && !wifiProvisioningMode()) {
    return Refusal{400, "setup only in AP mode"};
  }

  return std::nullopt;
}

void Refusal::send(WebServer& server) const {
  sendError(server, code, message);
}

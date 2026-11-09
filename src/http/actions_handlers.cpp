#include "http/handlers.h"

#include <cstring>

#include "actions/action.h"
#include "actions/action_list.h"
#include "actions/dispatch.h"
#include "http/request.h"
#include "http/response.h"
#include "settings/settings.h"

namespace {

JsonDocument actionsJson() {
  JsonDocument doc;
  doc["ok"] = true;
  JsonArray list = doc["actions"].to<JsonArray>();
  const ActionList& actions = settingsActions();

  for (size_t i = 0; i < actions.count(); i++) {
    const ActionEntry& entry = actions.at(i);
    JsonObject row = list.add<JsonObject>();
    row["on"] = entry.on;
    row["do"] = actionsDoName(entry.doKind);

    switch (entry.doKind) {
      case ActionDo::Sound:
        row["sound"] = actionsSoundName(entry.sound);
        break;
      case ActionDo::Anim:
        row["anim"] = entry.anim;
        break;
    }
  }

  return doc;
}

bool parseActionRow(JsonObjectConst obj, ActionEntry& out, const char*& error) {
  out = ActionEntry{};

  if (!obj["on"].is<const char*>() || !obj["do"].is<const char*>()) {
    error = "each action needs string on and do";
    return false;
  }

  const char* on = obj["on"];
  const char* doName = obj["do"];
  if (on == nullptr || doName == nullptr || on[0] == '\0') {
    error = "each action needs string on and do";
    return false;
  }
  if (strlen(on) > ACTIONS_ON_MAX_LEN) {
    error = "invalid on pattern";
    return false;
  }

  strncpy(out.on, on, ACTIONS_ON_MAX_LEN);
  out.on[ACTIONS_ON_MAX_LEN] = '\0';

  if (!actionsParseDo(doName, out.doKind)) {
    error = "invalid do";
    return false;
  }

  switch (out.doKind) {
    case ActionDo::Sound: {
      if (!obj["sound"].is<const char*>() ||
          !actionsParseSound(obj["sound"], out.sound)) {
        error = "sound action needs sound bell or click";
        return false;
      }
      break;
    }
    case ActionDo::Anim: {
      if (!obj["anim"].is<const char*>()) {
        error = "anim action needs string anim";
        return false;
      }
      const char* anim = obj["anim"];
      if (anim == nullptr || anim[0] == '\0' || strlen(anim) > ACTIONS_ANIM_MAX_LEN) {
        error = "invalid anim name";
        return false;
      }
      strncpy(out.anim, anim, ACTIONS_ANIM_MAX_LEN);
      out.anim[ACTIONS_ANIM_MAX_LEN] = '\0';
      break;
    }
  }

  if (!actionsValidateEntry(out)) {
    error = "invalid action";
    return false;
  }
  return true;
}

}  // namespace

void handleActionsGet(WebServer& server) {
  sendJson(server, 200, actionsJson());
}

void handleActionsPut(WebServer& server) {
  JsonDocument body;
  if (!readJsonBody<JsonObjectConst>(server, body)) {
    return;
  }

  if (!body["actions"].is<JsonArrayConst>()) {
    sendError(server, 400, "body must include actions array");
    return;
  }

  const JsonArrayConst rows = body["actions"].as<JsonArrayConst>();
  if (rows.size() > ACTIONS_MAX) {
    sendError(server, 400, "more than 16 actions");
    return;
  }

  ActionList next;
  for (JsonObjectConst row : rows) {
    ActionEntry entry{};
    const char* error = nullptr;
    if (!parseActionRow(row, entry, error) || !next.add(entry)) {
      sendError(server, 400, error != nullptr ? error : "invalid action");
      return;
    }
  }

  if (!saveActions(next)) {
    sendError(server, 500, "save failed");
    return;
  }

  sendJson(server, 200, actionsJson());
}

void handleActionsTest(WebServer& server) {
  JsonDocument body;
  if (!readJsonBody<JsonObjectConst>(server, body)) {
    return;
  }

  if (!body["type"].is<const char*>()) {
    sendError(server, 400, "missing type");
    return;
  }

  const char* type = body["type"];
  if (type == nullptr || type[0] == '\0') {
    sendError(server, 400, "missing type");
    return;
  }

  actionsDispatch(type);

  JsonDocument reply;
  reply["ok"] = true;
  reply["type"] = type;
  sendJson(server, 200, reply);
}

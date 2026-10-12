#include "api/dispatch.h"

#include <cstring>

#include "api/ops.h"

namespace {

bool pathEq(const char* path, const char* expect) {
  return std::strcmp(path, expect) == 0;
}

bool isAnimPath(const char* path) {
  return pathEq(path, "/anim");
}

bool isSettingsPath(const char* path) {
  return pathEq(path, "/settings") || pathEq(path, "/settings/reset");
}

bool isTestPath(const char* path) {
  return pathEq(path, "/test/audio") || pathEq(path, "/test/audio/bell") ||
         pathEq(path, "/test/screen") || pathEq(path, "/test/movement") ||
         pathEq(path, "/test/led") || pathEq(path, "/test/servo");
}

bool isSetupPath(const char* path) {
  return pathEq(path, "/setup/servo") || pathEq(path, "/setup/led") ||
         pathEq(path, "/setup/audio") || pathEq(path, "/setup/oled");
}

bool isServosPath(const char* path) {
  return pathEq(path, "/servos/park");
}

bool isKnownPath(const char* path) {
  return pathEq(path, "/auth") || pathEq(path, "/health") || isAnimPath(path) ||
         isSettingsPath(path) || isTestPath(path) || isSetupPath(path) ||
         isServosPath(path);
}

}  // namespace

void apiDispatch(const ApiRequest& req, ApiResponse& resp) {
  if (req.path[0] != '/') {
    resp.setError(404, "not found");
    return;
  }

  if (pathEq(req.path, "/auth")) {
    if (req.method != ApiMethod::Get) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpAuthGet(req, resp);
    return;
  }

  if (pathEq(req.path, "/health")) {
    if (req.method != ApiMethod::Get) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpHealthGet(req, resp);
    return;
  }

  if (pathEq(req.path, "/anim")) {
    if (req.method == ApiMethod::Get) {
      apiOpAnimGet(req, resp);
      return;
    }
    if (req.method == ApiMethod::Post) {
      apiOpAnimPost(req, resp);
      return;
    }
    resp.setError(405, "method not allowed");
    return;
  }

  if (pathEq(req.path, "/settings")) {
    if (req.method == ApiMethod::Get) {
      apiOpSettingsGet(req, resp);
      return;
    }
    if (req.method == ApiMethod::Post) {
      apiOpSettingsPost(req, resp);
      return;
    }
    resp.setError(405, "method not allowed");
    return;
  }

  if (pathEq(req.path, "/settings/reset")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpSettingsReset(req, resp);
    return;
  }

  if (pathEq(req.path, "/test/audio")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpTestAudio(req, resp);
    return;
  }

  if (pathEq(req.path, "/test/audio/bell")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpTestBell(req, resp);
    return;
  }

  if (pathEq(req.path, "/test/screen")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpTestScreen(req, resp);
    return;
  }

  if (pathEq(req.path, "/test/movement")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpTestMovement(req, resp);
    return;
  }

  if (pathEq(req.path, "/test/led")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpTestLed(req, resp);
    return;
  }

  if (pathEq(req.path, "/test/servo")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpTestServo(req, resp);
    return;
  }

  if (pathEq(req.path, "/setup/servo")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpSetupServo(req, resp);
    return;
  }

  if (pathEq(req.path, "/setup/led")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpSetupLed(req, resp);
    return;
  }

  if (pathEq(req.path, "/setup/audio")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpSetupAudio(req, resp);
    return;
  }

  if (pathEq(req.path, "/setup/oled")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpSetupOled(req, resp);
    return;
  }

  if (pathEq(req.path, "/servos/park")) {
    if (req.method != ApiMethod::Post) {
      resp.setError(405, "method not allowed");
      return;
    }
    apiOpServosPark(req, resp);
    return;
  }

  if (isKnownPath(req.path)) {
    resp.setError(405, "method not allowed");
    return;
  }

  resp.setError(404, "not found");
}

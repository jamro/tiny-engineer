#include "api/http_bridge.h"

#include <cstring>

#include "api/dispatch.h"
#include "http/json.h"

bool apiRequestFromWebServer(WebServer& server, ApiRequest& req) {
  req.clear();

  switch (server.method()) {
    case HTTP_GET:
      req.method = ApiMethod::Get;
      break;
    case HTTP_POST:
      req.method = ApiMethod::Post;
      break;
    case HTTP_PUT:
      req.method = ApiMethod::Put;
      break;
    default:
      req.method = ApiMethod::Other;
      break;
  }

  const String& uri = server.uri();
  std::strncpy(req.path, uri.c_str(), ApiRequest::kPathMax - 1);
  req.path[ApiRequest::kPathMax - 1] = '\0';

  const int n = server.args();
  for (int i = 0; i < n; i++) {
    const String name = server.argName(i);
    // WebServer exposes the body as "plain"; not a query param.
    if (name == "plain") {
      continue;
    }
    if (!req.addQuery(name.c_str(), server.arg(i).c_str())) {
      return false;
    }
  }
  return true;
}

void apiSendHttpResponse(WebServer& server, const ApiResponse& resp) {
  httpSendJson(server, resp.status, resp.body);
}

void httpDispatchApi(WebServer& server) {
  ApiRequest req;
  ApiResponse resp;
  if (!apiRequestFromWebServer(server, req)) {
    resp.setError(400, "query too large");
    apiSendHttpResponse(server, resp);
    return;
  }
  apiDispatch(req, resp);
  apiSendHttpResponse(server, resp);
}

#pragma once

#include <WebServer.h>

#include "api/request.h"

// Fill method/path/query from the active HTTP request.
void apiRequestFromWebServer(WebServer& server, ApiRequest& req);

// Send resp.status + resp.body as application/json (+ CORS).
void apiSendHttpResponse(WebServer& server, const ApiResponse& resp);

// Auth (and optional wifi) already applied by wrappers; build request and dispatch.
void httpDispatchApi(WebServer& server);

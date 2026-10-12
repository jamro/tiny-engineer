#pragma once

#include "api/request.h"

// Run a JSON control op. Auth is the caller's job (HTTP only).
// Wi-Fi-configured and setup-AP gates live inside ops.
void apiDispatch(const ApiRequest& req, ApiResponse& resp);

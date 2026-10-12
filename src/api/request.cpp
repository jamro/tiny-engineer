#include "api/request.h"

#include <cstdio>
#include <cstring>

void ApiRequest::clear() {
  method = ApiMethod::Other;
  path[0] = '\0';
  queryCount = 0;
}

bool ApiRequest::addQuery(const char* key, const char* value) {
  if (key == nullptr || value == nullptr || queryCount >= kQueryMax) {
    return false;
  }

  if (std::strlen(key) >= kKeyMax || std::strlen(value) >= kValueMax) {
    return false;
  }

  std::strncpy(keys[queryCount], key, kKeyMax - 1);
  keys[queryCount][kKeyMax - 1] = '\0';
  std::strncpy(values[queryCount], value, kValueMax - 1);
  values[queryCount][kValueMax - 1] = '\0';
  queryCount++;
  return true;
}

bool ApiRequest::hasArg(const char* key) const {
  if (key == nullptr) {
    return false;
  }

  for (size_t i = 0; i < queryCount; i++) {
    if (std::strcmp(keys[i], key) == 0) {
      return true;
    }
  }

  return false;
}

const char* ApiRequest::arg(const char* key) const {
  if (key == nullptr) {
    return "";
  }

  for (size_t i = 0; i < queryCount; i++) {
    if (std::strcmp(keys[i], key) == 0) {
      return values[i];
    }
  }

  return "";
}

void ApiResponse::set(int code, const char* json) {
  status = code;
  if (json == nullptr) {
    body[0] = '\0';
    return;
  }

  std::strncpy(body, json, kBodyMax - 1);
  body[kBodyMax - 1] = '\0';
}

void ApiResponse::setError(int code, const char* errorMessage) {
  status = code;
  const char* msg = errorMessage != nullptr ? errorMessage : "error";
  std::snprintf(
    body,
    kBodyMax,
    "{\"ok\":false,\"error\":\"%s\"}",
    msg
  );
}

bool apiParseOp(const char* op, ApiMethod& method, char* path, size_t pathSize) {
  if (op == nullptr || path == nullptr || pathSize == 0) {
    return false;
  }

  const char* space = std::strchr(op, ' ');
  if (space == nullptr || space == op || *(space + 1) != '/') {
    return false;
  }

  const size_t methodLen = static_cast<size_t>(space - op);
  if (methodLen == 3 && std::strncmp(op, "GET", 3) == 0) {
    method = ApiMethod::Get;
  } else if (methodLen == 4 && std::strncmp(op, "POST", 4) == 0) {
    method = ApiMethod::Post;
  } else if (methodLen == 3 && std::strncmp(op, "PUT", 3) == 0) {
    method = ApiMethod::Put;
  } else {
    method = ApiMethod::Other;
    return false;
  }

  const char* pathStart = space + 1;
  const size_t pathLen = std::strlen(pathStart);
  if (pathLen == 0 || pathLen >= pathSize) {
    return false;
  }

  std::memcpy(path, pathStart, pathLen + 1);
  return true;
}

const char* apiMethodName(ApiMethod method) {
  switch (method) {
    case ApiMethod::Get:
      return "GET";
    case ApiMethod::Post:
      return "POST";
    case ApiMethod::Put:
      return "PUT";
    default:
      return "OTHER";
  }
}

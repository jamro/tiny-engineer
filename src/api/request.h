#pragma once

#include <cstddef>
#include <cstdint>

// Transport-agnostic API request/response (HTTP + serial).

enum class ApiMethod : uint8_t {
  Get = 0,
  Post,
  Put,
  Other,
};

struct ApiRequest {
  static constexpr size_t kPathMax = 64;
  static constexpr size_t kQueryMax = 16;
  static constexpr size_t kKeyMax = 32;
  static constexpr size_t kValueMax = 80;

  ApiMethod method = ApiMethod::Other;
  char path[kPathMax] = {};
  size_t queryCount = 0;
  char keys[kQueryMax][kKeyMax] = {};
  char values[kQueryMax][kValueMax] = {};

  void clear();
  bool addQuery(const char* key, const char* value);
  bool hasArg(const char* key) const;
  // Missing → ""; never null.
  const char* arg(const char* key) const;
};

struct ApiResponse {
  static constexpr size_t kBodyMax = 3072;

  int status = 500;
  char body[kBodyMax] = {};

  void set(int code, const char* json);
  void setError(int code, const char* errorMessage);
};

// "POST /anim" → method + path. Returns false on bad form.
bool apiParseOp(const char* op, ApiMethod& method, char* path, size_t pathSize);

const char* apiMethodName(ApiMethod method);

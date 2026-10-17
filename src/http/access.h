#pragma once

#include <WebServer.h>

#include <optional>

enum class Access {
  Public,
  Token,
  // Token, and refused with 503 until connection mode is set (Wi-Fi or USB-only).
  Configured,
  // Token, and only in setup AP mode: these routes drive the hardware without the saved limits.
  Setup,
};

// Why a request may not reach its route.
struct Refusal {
  int code;
  const char* message;

  static std::optional<Refusal> check(WebServer& server, Access access);
  void send(WebServer& server) const;
};

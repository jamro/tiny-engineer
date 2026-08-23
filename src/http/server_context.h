#pragma once

#include <WebServer.h>

WebServer& httpServer();
void refreshMdnsHostname();
const char* httpMdnsHostname();

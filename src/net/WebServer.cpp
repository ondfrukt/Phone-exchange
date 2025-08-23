#include "Webserver.h"
#include "LineManager.h"

#include "WebServer.h"
#include "LineManager.h"

WebServer::WebServer(LineManager& lineManager) : lineMgr_(lineManager) {}

void WebServer::start() {
  // startlogik här vid behov
}

void WebServer::handleClient() {
  // hantering av klienter här vid behov
}

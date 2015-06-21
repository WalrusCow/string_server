#pragma once

#include <list>
#include <string>
#include <sstream>

#include <netinet/in.h>

#include "Connection.hpp"

std::string titleCase(const std::string& inString);

class Server {
 public:
  void run();
  void connect();

 private:
  int mainSocket;
  struct sockaddr_in sin;
  socklen_t sinLen = sizeof(sin);

  std::list<Connection> connections;
  fd_set readSet;

  void waitForActivity();
  void checkForNewConnections();
  void handleConnections();
};

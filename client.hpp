#pragma once

#include <atomic>
#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "ThreadQueue.hpp"

class Client {
 public:
  Client(const std::string& hostname, int port);
  void sendFrom(std::istream& is);

 private:
  void sendString(const std::string& str);
  // Thread worker
  void sendStrings(ThreadQueue& stringQueue,
                   const std::atomic_bool& inputDone);

  struct hostent* server;
  struct sockaddr_in addr;
  int sfd;
};

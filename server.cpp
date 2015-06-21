#include "server.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <list>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Connection.hpp"

namespace {

void fatalError(const std::string& error, int exitCode) {
  std::cerr << error << std::endl;
  std::exit(exitCode);
}

} // Anonymous

std::string titleCase(const std::string& inString) {
  bool toUpper = true;
  std::stringstream ss;

  for (const char& c : inString) {
    // Type is actually int, so we must cast
    ss << static_cast<char>(toUpper ? std::toupper(c) : std::tolower(c));
    toUpper = (c == ' ');
  }
  return ss.str();
}

int main(void) {
  int mainSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (mainSocket == -1) {
    fatalError("Failed to create socket", -1);
  }

  char hostnameBuffer[1024];
  gethostname(hostnameBuffer, sizeof(hostnameBuffer));
  hostnameBuffer[sizeof(hostnameBuffer) - 1] = '\0';
  std::cout << "SERVER_ADDRESS " << hostnameBuffer << std::endl;

  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = 0; // Next available

  if (bind(mainSocket, (struct sockaddr*) (&sin), len) == -1) {
    fatalError("Failed to bind socket", -2);
  }

  // Listen with max 5 pending connections
  if (listen(mainSocket, 5) == -1) {
    fatalError("Failed to listen on socket", -3);
  }

  // Get port number
  if (getsockname(mainSocket, (struct sockaddr*) (&sin), &len) == -1) {
    fatalError("Could not get socket name", -4);
  }

  std::cerr << "SERVER_PORT " << ntohs(sin.sin_port) << std::endl;

  std::list<Connection> connections;
  fd_set readSet;
  char buffer[1025];

  while(true) {
    FD_ZERO(&readSet);
    FD_SET(mainSocket, &readSet);

    int maxFd = mainSocket;

    for (const auto& connection : connections) {
      FD_SET(connection.socket, &readSet);
      maxFd = std::max(maxFd, connection.socket);
    }

    if (select(maxFd + 1, &readSet, nullptr, nullptr, nullptr) < 0) {
      fatalError("Error from select", -5);
    }
    std::cout << "Past select" << std::endl;

    if (FD_ISSET(mainSocket, &readSet)) {
      // Activity on the main socket means that there is a new client
      auto newSocket = accept(mainSocket, (struct sockaddr*)&sin, &len);
      std::cout << "New connection" << std::endl;
      if (newSocket < 0) {
        // Error
        std::cerr << "Error accepting new socket" << std::endl;
      }
      connections.emplace_back(newSocket);
    }

    // Check clients for activity too
    auto i = connections.begin();
    while (i != connections.end()) {
      auto& connection = *i;
      if (!FD_ISSET(connection.socket, &readSet)) {
        i++;
        continue;
      }

      std::string receivedMessage;
      int finished = connection.read(receivedMessage);
      if (finished < 0) {
        // error
        connection.close();
        i = connections.erase(i);
        continue;
      }
      if (!finished) {
        i++;
        continue;
      }

      // Done reading. Remove from queue
      i = connections.erase(i);
      std::string reply = titleCase(receivedMessage);
      connection.send(reply);
      connection.close();
    }
  }
}

#include "server.hpp"

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <sstream>

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

void Server::connect() {
  // Connect to port
  mainSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (mainSocket == -1) {
    fatalError("Failed to create socket", -1);
  }

  char hostnameBuffer[1024];
  gethostname(hostnameBuffer, sizeof(hostnameBuffer));
  hostnameBuffer[sizeof(hostnameBuffer) - 1] = '\0';
  std::cout << "SERVER_ADDRESS " << hostnameBuffer << std::endl;

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = 0; // Next available
  sin.sin_port = 12818;

  if (bind(mainSocket, (struct sockaddr*) (&sin), sinLen) == -1) {
    fatalError("Failed to bind socket", -2);
  }

  // Listen with max 5 pending connections
  if (listen(mainSocket, 5) == -1) {
    fatalError("Failed to listen on socket", -3);
  }

  // Get port number
  if (getsockname(mainSocket, (struct sockaddr*) (&sin), &sinLen) == -1) {
    fatalError("Could not get socket name", -4);
  }

  std::cout << "SERVER_PORT " << ntohs(sin.sin_port) << std::endl;
}

void Server::waitForActivity() {
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
}

void Server::checkForNewConnections() {
  if (!FD_ISSET(mainSocket, &readSet)) {
    return;
  }

  // Activity on the main socket means that there is a new client
  auto newSocket = accept(mainSocket, (struct sockaddr*)&sin, &sinLen);
  if (newSocket < 0) {
    // Error
    std::cerr << "Error accepting new socket" << std::endl;
  }
  connections.emplace_back(newSocket);
}

void Server::handleConnections() {
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
      std::cerr << "Error on reading" << std::endl;
      connection.close();
      i = connections.erase(i);
      continue;
    }
    if (!finished) {
      i++;
      continue;
    }

    std::string reply = titleCase(receivedMessage);
    connection.send(reply);
    connection.close();

    // Done reading. Remove from queue
    i = connections.erase(i);
  }
}

void Server::run() {
  while(true) {
    waitForActivity();
    checkForNewConnections();
    handleConnections();
  }
}

int main(void) {
  Server server;
  server.connect();
  server.run();
}

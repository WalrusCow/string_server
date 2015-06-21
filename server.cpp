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

namespace {

void fatalError(const std::string& error, int exitCode) {
  std::cerr << error << std::endl;
  std::exit(exitCode);
}

} // Anonymous

void Connection::close() {
  if (!valid) {
    std::cerr << "Attempting to close closed connection" << std::endl;
  }
  ::close(socket);
  valid = false;
}

int Connection::send(const std::string& reply) {
  uint32_t length = reply.size() + 1;
  const uint32_t BUFFER_LEN = 1024;
  char buffer[BUFFER_LEN];
  std::memcpy(buffer, &length, sizeof(length));
  auto bytesWritten = write(socket, buffer, sizeof(length));
  if (bytesWritten < 4) {
    // Could not write length
    std::cerr << "Could not write length " <<bytesWritten << std::endl;
    return -1;
  }

  const char* cStr = reply.c_str();

  uint32_t toWrite = length;
  while (toWrite > 0) {
    std::cerr << "writing some stuff" << std::endl;
    // While we have not written everything yet
    // Copy remaining into buffer
    auto toCopy = std::min(BUFFER_LEN, toWrite);
    std::memcpy(buffer, cStr, toCopy);
    bytesWritten = write(socket, buffer, toCopy);
    if (bytesWritten < 0) {
      // Error
      return -1;
    }
    // We have written some bytes
    cStr += bytesWritten;
    toWrite -= bytesWritten;
  }
  return 0;
}

int Connection::read(std::string& result) {
  if (!valid) {
    std::cerr << "Attempting to read from invalid connection" << std::endl;
    return -1;
  }

  const uint32_t BUFFER_LEN = 1024;
  char buffer[BUFFER_LEN];

  ssize_t bytesReceived;
  if (messageLength == 0) {
    // We have to read the messagelength
    bytesReceived = ::read(socket, buffer, 4);
    if (bytesReceived < 4) {
      // Bad. Should handle this better, probably
      std::cerr << "Could not read message length " <<bytesReceived<< std::endl;
      // Error
      return -1;
    }
  }

  uint32_t toRead;
  uint32_t bytesToLoad;
  do {
    toRead = messageLength - bytesRead;
    // Read as much as we can
    bytesToLoad = std::min(BUFFER_LEN, toRead);
    bytesReceived = ::read(socket, buffer, bytesToLoad);
    toRead -= bytesReceived;
    ss.read(buffer, bytesReceived);
    std::cout << "server Reading..." << std::endl;
  } while (bytesReceived > 0 && toRead > 0);

  // We have read the whole message
  if (toRead <= 0) {
    std::cout << "server Done reading" <<std::endl;
    result = ss.str();
    // Done
    return 1;
  }
  std::cout << "sever not done reading" <<std::endl;
  // Not done
  return 0;
}

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

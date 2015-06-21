#include "server.hpp"

#include <cctype>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>

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

  std::vector<int> clientSockets;
  fd_set readSet;
  char buffer[1025];

  while(true) {
    FD_ZERO(&readSet);
    FD_SET(mainSocket, &readSet);

    int maxFd = mainSocket;

    for (auto clientSocket : clientSockets) {
      FD_SET(clientSocket, &readSet);
      maxFd = std::max(maxFd, clientSocket);
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
      clientSockets.push_back(newSocket);
    }

    // Check clients for activity too
    auto i = clientSockets.begin();
    while (i != clientSockets.end()) {
      auto clientSocket = *i;
      if (!FD_ISSET(clientSocket, &readSet)) {
        i += 1;
        continue;
      }

      // TODO: Read 4 bytes then try to read more.
      // TODO: Read as much as possible. Not just 1024. Read in a lewp.
      auto bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
      if (bytesRead == 0) {
        // Error or EOF
        i = clientSockets.erase(i);
        close(clientSocket);
      }
      else {
        buffer[sizeof(buffer) - 1] = '\0';
        i += 1;
        // TODO: Data is in buffer. Put into stringstream for this client
        uint32_t strLen;
        std::memcpy(&strLen, buffer, sizeof(strLen));
        std::cout << "string length " << strLen << std::endl;
        std::cout << "got " << bytesRead << " bytes: " <<buffer+4<<std::endl;
        send(clientSocket, "hello", 6, 0);
        //send(clientSocket, buffer, strlen(buffer), 0);
      }


    }

  }
}

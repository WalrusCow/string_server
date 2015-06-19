#include "server.hpp"

#include <cctype>
#include <iostream>
#include <cstdlib>
#include <sstream>

#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

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
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    fatalError("Failed to create socket", -1);
  }

  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = 0; // Next available

  if (bind(sfd, (struct sockaddr*) (&sin), len) == -1) {
    fatalError("Failed to bind socket", -2);
  }

  // Listen with max 5 pending connections
  if (listen(sfd, 5) == -1) {
    fatalError("Failed to listen on socket", -3);
  }

  // Get port number
  if (getsockname(sfd, (struct sockaddr*) (&sin), &len) == -1) {
    fatalError("Could not get socket name", -4);
  }

  std::cerr << "Listening on port " << ntohs(sin.sin_port) << std::endl;

  fd_set readSet;
  FD_ZERO(&readSet);
  while(1) {
    select(0, &readSet, nullptr, nullptr, nullptr);
    //if (FD_ISSET(
  }
}

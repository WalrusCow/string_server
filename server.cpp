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
  struct sockaddr_in serveAddr;
  if (sfd == -1) {
    fatalError("Failed to create socket", -1);
  }

  serveAddr.sin_family = AF_INET;
  serveAddr.sin_addr.s_addr = INADDR_ANY;
  serveAddr.sin_port = 0; // Next available

  if (bind(sfd, (struct sockaddr*) (&serveAddr), sizeof(serveAddr)) == -1) {
    fatalError("Failed to bind socket", -2);
  }

  while(1) {
    
  }
}

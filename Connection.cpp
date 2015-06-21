#include "Connection.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

#include <sys/socket.h>
#include <unistd.h>

Connection::Connection() : Connection(0) {}

Connection::Connection(int socket_) : socket(socket_) {}

void Connection::close() {
  ::close(socket);
}

int Connection::send(const std::string& str) {
  uint32_t length = str.size() + 1;
  const uint32_t BUFFER_LEN = 1024;
  char buffer[BUFFER_LEN];
  std::memcpy(buffer, &length, sizeof(length));
  auto bytesWritten = write(socket, buffer, sizeof(length));
  if (bytesWritten < 4) {
    // Could not write length
    return -1;
  }

  const char* cStr = str.c_str();

  uint32_t toWrite = length;
  while (toWrite > 0) {
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

int Connection::doRead(
    std::string& result,
    const std::function<ssize_t(int, char*, size_t)>& reader) {

  const uint32_t BUFFER_LEN = 1024;
  char buffer[BUFFER_LEN];

  ssize_t bytesReceived;
  if (messageLength == 0) {
    // We have to read the messagelength
    bytesReceived = reader(socket, buffer, 4);
    if (bytesReceived == 0) {
      // Not done
      return 0;
    }
    if (bytesReceived < 4) {
      // Error
      return -1;
    }
    std::memcpy(&messageLength, buffer, 4);
  }

  uint32_t toRead;
  uint32_t bytesToLoad;
  do {
    toRead = messageLength - bytesRead;
    // Read as much as we can
    bytesToLoad = std::min(BUFFER_LEN, toRead);
    bytesReceived = reader(socket, buffer, bytesToLoad);
    if (bytesReceived < 0) {
      return -1;
    }

    bytesRead += bytesReceived;
    toRead -= bytesReceived;
    // Add to string buffer
    ss.write(buffer, bytesReceived);
  } while (bytesReceived > 0 && toRead > 0);

  // We have read the whole message
  if (toRead <= 0) {
    result = ss.str();
    // Done
    return 1;
  }
  // Not done
  return 0;
}

int Connection::read(std::string& result) {
  return doRead(result, [&] (int sfd, char* buffer, size_t toRead) {
    return ::read(sfd, buffer, toRead);
  });
}

int Connection::recv(std::string& result) {
  return doRead(result, [&] (int sfd, char* buffer, size_t toRead) {
      return ::recv(sfd, buffer, toRead, 0);
  });
}

void Connection::useSocket(int socket_) {
  socket = socket_;
  ss.str("");
  ss.clear();
  messageLength = 0;
  bytesRead = 0;
}

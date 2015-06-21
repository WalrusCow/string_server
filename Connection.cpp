#include "Connection.hpp"

#include <algorithm>
#include <sys/socket.h>
#include <iostream>
#include <cstring>

#include <unistd.h>

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

int Connection::doRead(
    std::string& result,
    const std::function<ssize_t(int, char*, size_t)>& reader) {
  if (!valid) {
    std::cerr << "Attempting to read from invalid connection" << std::endl;
    return -1;
  }

  const uint32_t BUFFER_LEN = 1024;
  char buffer[BUFFER_LEN];

  ssize_t bytesReceived;
  if (messageLength == 0) {
    // We have to read the messagelength
    bytesReceived = reader(socket, buffer, 4);
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
    bytesReceived = reader(socket, buffer, bytesToLoad);
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

int Connection::read(std::string& result) {
  return doRead(result, [&] (int socket, char* buffer, size_t toRead) {
    return ::read(socket, buffer, toRead);
  });
}

int Connection::recv(std::string& result) {
  return doRead(result, [&] (int socket, char* buffer, size_t toRead) {
      return ::recv(socket, buffer, toRead, 0);
  });
}

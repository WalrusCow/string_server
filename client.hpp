#pragma once

#include <atomic>
#include <iostream>
#include <queue>
#include <string>

class Client {
 public:
  void sendFrom(std::istream& is);

 private:
  void sendString(const std::string& str);
  // Thread worker
  void sendStrings(std::queue<std::string>& stringQueue,
                   const std::atomic_bool& inputDone);
};

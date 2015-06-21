#pragma once

#include <queue>
#include <string>
#include <mutex>

class ThreadQueue {
 public:
  void pop();
  std::string front();
  bool empty();
  void push(const std::string& s);
 private:
  std::mutex accessMutex;
  std::queue<std::string> queue;
};

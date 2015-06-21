#include "ThreadQueue.hpp"

void ThreadQueue::pop() {
  accessMutex.lock();
  queue.pop();
  accessMutex.unlock();
}

std::string ThreadQueue::front() {
  accessMutex.lock();
  auto s = queue.front();
  accessMutex.unlock();
  return s;
}

bool ThreadQueue::empty() {
  accessMutex.lock();
  auto r = queue.empty();
  accessMutex.unlock();
  return r;
}

void ThreadQueue::push(const std::string& s) {
  accessMutex.lock();
  queue.push(s);
  accessMutex.unlock();
}

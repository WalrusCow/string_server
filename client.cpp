#include "client.hpp"

#include <cstring>
#include <thread>

void Client::sendFrom(std::istream& is) {
  std::queue<std::string> stringQueue;
  std::atomic_bool finishedFlag(false);

  // spawn thread for things
  std::thread thread(&Client::sendStrings,
                     this,
                     std::ref(stringQueue),
                     std::ref(finishedFlag));

  std::string line;
  while (std::getline(is, line)) {
    stringQueue.push(line);
  }

  finishedFlag.store(true);
  thread.join();
}

void Client::sendStrings(std::queue<std::string>& stringQueue,
                         const std::atomic_bool& inputDone) {
  // While we have not finished reading input or there is a queue...
  while (!inputDone.load() || !stringQueue.empty()) {

    // Wait for the queue to be non-empty or for us to finish
    while (stringQueue.empty() && !inputDone.load()) {
      // Wait for more in the queue if it is empty and we are not done
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Queue is empty and no more input: stop.
    if (inputDone.load() && stringQueue.empty()) return;

    // Do next one
    sendString(stringQueue.front());
    stringQueue.pop();

    if (!inputDone.load() || !stringQueue.empty()) {
      // Not done yet: sleep 2 seconds
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  }
}

void Client::sendString(const std::string& str) {
  uint32_t len = str.size();
  char buf[1024];
  std::memcpy(buf, &len, sizeof(len));
}

int main(void) {
  Client client;
  client.sendFrom(std::cin);
}

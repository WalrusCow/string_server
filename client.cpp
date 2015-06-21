#include "client.hpp"

#include <cstring>
#include <thread>

#include <unistd.h>

Client::Client(const std::string& hostname, int port) {
  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd < 0) {
    std::cerr << "Error opening socket" << std::endl;
  }
  server = gethostbyname(hostname.c_str());
  addr.sin_family = AF_INET;

  bcopy((char*)server->h_addr,
        (char*)&addr.sin_addr.s_addr,
        (int)server->h_length);
  addr.sin_port = htons(port);

  if (connect(sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    std::cerr << "Error connecting to server" << std::endl;
  }
}

void Client::sendFrom(std::istream& is) {
  ThreadQueue stringQueue;
  std::atomic_bool finishedFlag(false);

  // spawn thread for things
  std::thread thread(&Client::sendStrings,
                     this,
                     std::ref(stringQueue),
                     std::ref(finishedFlag));

  std::string line;
  while (std::getline(is, line)) {
    std::cout << line << std::endl;
    stringQueue.push(line);
  }

  finishedFlag.store(true);
  thread.join();
}

void Client::sendStrings(ThreadQueue& stringQueue,
                         const std::atomic_bool& inputDone) {
  // While we have not finished reading input or there is a queue...
  while (!inputDone.load() || !stringQueue.empty()) {

    // Wait for the queue to be non-empty or for us to finish
    while (stringQueue.empty() && !inputDone.load()) {
      // Wait for more in the queue if it is empty and we are not done
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Queue is empty and no more input: stop.
    if (inputDone.load() && stringQueue.empty()) {
      return;
    }

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
  // Four bits
  uint32_t len = str.size() + 1;
  char buf[1024];
  std::memcpy(buf, &len, sizeof(len));

  // Include null terminator
  auto messageSize = str.size() + 1 + sizeof(len);
  auto n = write(sfd, buf, messageSize);
  if (n < 0) {
    // Error
    std::cerr << "Error when writing to socket" << std::endl;
  }
  std::cout << "Wrote " << n << "bytes"<<std::endl;
  // Write in loop.. may not be able to write all

  // Wait for response
  auto buf2 = std::unique_ptr<char>(new char[messageSize]);
  recv(sfd, buf2.get(), messageSize, 0);

  std::string message(buf2.get() + sizeof(len));
  std::cout << "Got response from server:" << message << std::endl;
}

int main(void) {
  std::string hostname = getenv("SERVER_ADDRESS");
  int port = std::stoi(getenv("SERVER_PORT"));
  Client client(hostname, port);
  client.sendFrom(std::cin);
}

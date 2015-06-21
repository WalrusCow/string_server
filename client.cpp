#include "client.hpp"

#include <cstring>
#include <thread>

#include <unistd.h>

#include "ThreadQueue.hpp"

Client::Client(const std::string& hostname, int port) {
  server = gethostbyname(hostname.c_str());
  addr.sin_family = AF_INET;

  bcopy((char*)server->h_addr,
        (char*)&addr.sin_addr.s_addr,
        (int)server->h_length);
  addr.sin_port = htons(port);
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
  connect();
  connection.send(str);

  std::string reply;
  connection.recv(reply);
  std::cout << "Server: " << reply << std::endl;

  connection.close();
}

void Client::connect() {
  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd < 0) {
    std::cerr << "Error opening socket" << std::endl;
  }

  if (::connect(sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    std::cerr << "Error connecting to server" << std::endl;
  }

  connection.useSocket(sfd);
}

int main(void) {
  std::string hostname = getenv("SERVER_ADDRESS");
  int port = std::stoi(getenv("SERVER_PORT"));
  Client client(hostname, port);
  client.sendFrom(std::cin);
}

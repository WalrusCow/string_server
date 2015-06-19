#include "server.hpp"

#include <cctype>
#include <iostream>
#include <sstream>

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
  std::string s = titleCase("");
  std::cout << s << std::endl;
}

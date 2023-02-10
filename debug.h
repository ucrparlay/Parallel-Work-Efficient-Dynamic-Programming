#ifndef DEBUG_H_
#define DEBUG_H_

#include <iostream>
#include <string>

#include "parlay/sequence.h"

template <typename Seq>
void debug(const Seq& s) {
  std::cout << "size: " << s.size() << ", [";
  for (unsigned int i = 0; i < s.size(); i++) {
    std::cout << s[i];
    if (i != s.size() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << "]" << std::endl;
}

template <typename Seq>
void debug(std::string msg, const Seq& s) {
  std::cout << msg << ": ";
  debug(s);
}

#endif  // DEBUG_H_
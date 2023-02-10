#include <iostream>

#include "parlay/sequence.h"

using namespace std;

int main() {
  parlay::sequence<int> a(100);
  cout << a[0] << endl;
  return 0;
}
#include <iostream>
#include <random>

#include "dc3.h"
#include "parlay/internal/get_time.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"

using namespace std;

bool good(int n, int alpha) {
  std::mt19937 rng(0);
  auto a = parlay::sequence<unsigned int>(n);
  for (int i = 0; i < n; i++) {
    a[i] = rng() % alpha + 1;
  }
  double x = 0, y = 0;
  for (int rr = 0; rr < 5; rr++) {
    parlay::internal::timer tt;
    auto sa1 = suffix_array(a);
    x += tt.next_time();
    auto sa2 = DC3(a);
    y += tt.stop();
    if (rr > 0) continue;
    for (int i = 0; i < n; i++) {
      if (sa1[i] != sa2[i]) return false;
    }
  }
  x /= 5, y /= 5;
  cout << x << endl << y << endl << "rate: " << y / x << endl;
  return true;
}

int main() {
  assert(good(100000, 128));
  cout << "SA test pass!" << endl;
  return 0;
}
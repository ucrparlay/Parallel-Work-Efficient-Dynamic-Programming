#include <iostream>
#include <random>

#include "dc3.h"
#include "debug.h"
#include "parlay/internal/get_time.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"
#include "suffix_array_parallel.h"

using namespace std;

std::mt19937 rng(0);

bool good(int n, int alpha) {
  auto a = parlay::sequence<unsigned int>(n);
  for (int i = 0; i < n; i++) {
    a[i] = rng() % alpha + 1;
  }
  auto b = parlay::sequence<unsigned int>(n * 2);
  for (int i = 0; i < n; i++) {
    b[i] = b[i + n] = a[i];
    if (rng() % 100 >= 0) {
      b[i + n] = rng() % alpha + 1;
    }
  }
  double x = 0, y = 0;
  for (int rr = 0; rr < 5; rr++) {
    parlay::internal::timer tt;
    auto [rank1, sa1, lcp1] = suffix_array_large_alphabet(b);
    x += tt.next_time();
    auto [rank2, sa2, lcp2] = DC3(b);
    y += tt.stop();
    if (rr > 0) continue;
    for (int i = 0; i < n * 2; i++) {
      if (sa1[i] != sa2[i] || rank1[i] != rank2[i] || lcp1[i] != lcp2[i])
        return false;
    }
  }
  x /= 5, y /= 5;
  cout << x << endl << y << endl << "rate: " << y / x << endl;
  return true;
}

int main() {
  int n = 1000000;
  parlay::sequence<unsigned int> a(n);
  for (int i = 0; i < n; i++) {
    a[i] = rng() % 200000000;
  }
  auto b = a;
  parlay::internal::timer tt;
  parlay::stable_sort_inplace(a);
  cout << tt.next_time() << endl;
  parlay::stable_integer_sort_inplace(parlay::make_slice(b),
                                      [&](unsigned int x) { return x; });
  cout << tt.stop() << endl;
  for (int i = 0; i < n; i++) {
    assert(a[i] == b[i]);
  }
  cout << "Sorting test pass!" << endl;

  assert(good(1000000, 1000000));
  cout << "SA test pass!" << endl;

  return 0;
}
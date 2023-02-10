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
  parlay::internal::timer tt;
  tt.start();
  auto [rank1, sa1, lcp1] = suffix_array_large_alphabet(a);
  cout << tt.next_time() << endl;
  auto [rank2, sa2, lcp2] = DC3(a);
  cout << tt.stop() << endl;
  for (int i = 0; i < n; i++) {
    if (sa1[i] != sa2[i]) return false;
  }
  return true;
}

int main() {
  // int n = 100000;
  // parlay::sequence<unsigned int> a(n);
  // for (int i = 0; i < n; i++) {
  //   a[i] = rng() % 200000000;
  // }
  // auto b = a;
  // parlay::internal::timer tt;
  // parlay::stable_sort_inplace(a);
  // cout << tt.next_time() << endl;
  // parlay::stable_integer_sort_inplace(parlay::make_slice(b),
  //                                     [&](unsigned int x) { return x; });
  // cout << tt.stop() << endl;
  // for (int i = 0; i < n; i++) {
  //   assert(a[i] == b[i]);
  // }
  // cout << "Sorting test pass!" << endl;

  good(1000000, 2);
  cout << "SA test pass!" << endl;

  // parlay::sequence<unsigned int> a = {1,3,5,7,1};
  // DC3(a);

  return 0;
}
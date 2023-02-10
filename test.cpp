#include <iostream>

#include "parlay/internal/get_time.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"

using namespace std;

int main() {
  int n = 100000000;
  parlay::sequence<unsigned int> a(n);
  for (int i = 0; i < n; i++) {
    a[i] = rand() % 200000000;
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
  cout << "Pass!" << endl;
  return 0;
}
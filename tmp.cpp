#include "parlay/internal/get_time.h"
#include "parlay/sequence.h"
int main() {
  int n = 1e7;
  auto f = []() {};
  parlay::internal::timer t1;
  for (int i = 0; i < n; i++) {
    parlay::parallel_do(f, f);
  }
  t1.total();
  parlay::internal::timer t2;
  for (int i = 0; i < n; i++) {
    f(), f();
  }
  t2.total();
  return 0;
}
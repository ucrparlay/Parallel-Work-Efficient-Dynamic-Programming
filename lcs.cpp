#include <iostream>

#include "gflags/gflags.h"
#include "lcs_brute_force.h"
#include "lcs_parallel.h"
#include "parlay/sequence.h"

using namespace std;

DEFINE_uint64(n, 10, "n");
DEFINE_uint64(range, 10, "range");
DEFINE_bool(lis, false, "lis");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto n = FLAGS_n;
  parlay::sequence<size_t> a(n + 1), b(n + 1);
  parlay::parallel_for(
      1, n + 1, [&](size_t i) { a[i] = parlay::hash64(i) % FLAGS_range; });
  if (FLAGS_lis) {
    parlay::parallel_for(1, n + 1, [&](size_t i) { b[i] = i; });
  } else {
    parlay::parallel_for(1, n + 1, [&](size_t i) {
      b[i] = parlay::hash64(parlay::hash64(i) + 1) % FLAGS_range;
    });
  }

  auto res1 = BruteForceLCS(a, n, b, n);
  std::cout << "res1: " << res1 << std::endl;

  auto res2 = ParallelLCS(a, n, b, n);
  std::cout << "res2: " << res2 << std::endl;

  std::cout << "\nok: " << (res1 == res2) << std::endl;

  return 0;
}

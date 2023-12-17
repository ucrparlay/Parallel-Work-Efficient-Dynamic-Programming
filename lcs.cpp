#include <iostream>

#include "gflags/gflags.h"
#include "lcs_brute_force.h"
#include "lcs_parallel.h"
#include "parlay/sequence.h"

using namespace std;

DEFINE_uint64(n, 10, "n");
DEFINE_uint64(range, 10, "range");
DEFINE_bool(lis, false, "lis");
DEFINE_string(run, "bf,par", "bf, par");

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

  parlay::internal::timer tm;
  size_t res1, res2;

  if (FLAGS_run.find("bf") != string::npos) {
    res1 = BruteForceLCS(a, n, b, n);
    std::cout << "bf res: " << res1 << std::endl;
    tm.next("brute force");
  }

  if (FLAGS_run.find("par") != string::npos) {
    res2 = ParallelLCS(a, n, b, n);
    std::cout << "par res: " << res2 << std::endl;
    tm.next("parallel");
  }

  std::cout << "\nok: " << (res1 == res2) << std::endl;

  return 0;
}

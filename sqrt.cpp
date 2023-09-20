#include <iostream>
#include <random>

#include "brute_force_dp.h"
#include "concave_dp_parallel.h"
#include "concave_dp_sequential.h"
#include "config.h"
#include "gflags/gflags.h"
#include "parlay/internal/get_time.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"
#include "parlay/utilities.h"

using namespace std;

using real = long double;

DEFINE_uint64(n, 10, "n");
DEFINE_uint64(range, 100, "range");
DEFINE_double(cost, 2, "cost");
DEFINE_string(run, "seq,par", "brute, seq, par");

auto MakeData(size_t n) {
  parlay::sequence<real> a(n + 1);
  parlay::parallel_for(1, n + 1, [&](size_t i) {
    a[i] = parlay::hash64(parlay::hash64(n) ^ parlay::hash64(FLAGS_range) ^
                          parlay::hash64(i)) %
           FLAGS_range;
  });
  return a;
}

int main(int argc, char *argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  size_t n = FLAGS_n;
  real cost = FLAGS_cost;
  cout << "n: " << n << endl;
  cout << "cost: " << cost << endl;

  cout << "MakeData start" << endl;
  parlay::sequence<real> a = MakeData(n);
  // cout << "a: ";
  // for (size_t i = 1; i <= n; i++) cout << a[i] << " \n"[i == n];
  auto sum = parlay::scan_inclusive(a);
  assert(sum.size() == n + 1);
  cout << "MakeData end" << endl;

  // D[i] = E[i] - cost
  auto f = [&](real Ei) -> real { return Ei - cost; };

  // w(i, j) = sqrt(sum(i + 1, j))
  auto w = [&](size_t i, size_t j) -> real {
    assert(i < j);
    return sqrt(sum[j] - sum[i]);
  };

  parlay::internal::timer tm;

  parlay::sequence<real> E1(n + 1);
  parlay::sequence<real> E2(n + 1);
  parlay::sequence<real> E3(n + 1);

  // BruteForceDP(n, E1, f, w);
  // tm.next("brute-force");

  // cout << "E1: ";
  // for (int i = 1; i <= n; i++) {
  //   cout << fixed << setprecision(6) << E1[i] << " \n"[i == n];
  // }

  if (FLAGS_run.find("seq") != string::npos) {
    ConcaveDPSequential(n, E2, f, w);
    tm.next("sequential");
  }

  if (FLAGS_run.find("par") != string::npos) {
    ConcaveDPParallel(n, E3, f, w);
    tm.next("parallel");
  }

  bool ok = parlay::all_of(parlay::iota(n + 1), [&](size_t i) {
    // return abs(E1[i] - E2[i]) < 1e-7 && abs(E2[i] - E3[i]) < 1e-7;
    return abs(E2[i] - E3[i]) < 1e-7;
  });
  cout << "ok: " << ok << endl;

  for (size_t i = 1; i <= n; i++) {
    if (abs(E2[i] - E3[i]) > 1e-7) {
      cout << i << ' ' << E2[i] << ' ' << E3[i] << endl;
    }
  }

  return 0;
}
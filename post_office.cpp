#include <iostream>
#include <random>

#include "brute_force_dp.h"
#include "config.h"
#include "convex_dp_new.h"
#include "convex_dp_new2.h"
#include "convex_dp_parallel.h"
#include "convex_dp_sequential.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "parlay/internal/get_time.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"
#include "parlay/utilities.h"

using namespace std;

using real = long double;

DEFINE_uint64(n, 10, "n");
DEFINE_uint64(range, 100, "range");
DEFINE_double(cost, 10, "cost");
DEFINE_string(run, "par,new1,new2", "bf, seq, par, new1, new2");

auto MakeData(size_t n) {
  parlay::sequence<real> x(n + 1);
  parlay::parallel_for(1, n + 1, [&](size_t i) {
    x[i] = parlay::hash64(parlay::hash64(n) ^ parlay::hash64(FLAGS_range) ^
                          parlay::hash64(i)) %
               FLAGS_range +
           1;
  });
  parlay::sort_inplace(x);
  return x;
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  // FLAGS_log_dir = CMAKE_CURRENT_SOURCE_DIR "/logs";
  FLAGS_alsologtostderr = true;
  google::InitGoogleLogging(argv[0]);

  size_t n = FLAGS_n;
  real cost = FLAGS_cost;

  LOG(INFO) << "\nPost Office  "
            << "n = " << n << '\n';

  auto x = MakeData(n);
  if (n <= 20) {
    cout << "x: ";
    for (int i = 1; i <= n; i++) cout << x[i] << " \n"[i == n];
  }
  cout << "cost: " << cost << '\n';
  auto sum = parlay::scan_inclusive(x);

  // D[i] = E[i] + cost
  auto f = [&](real Ei) -> real { return Ei + cost; };

  // E[j] = min(D[i] + w(i, j))
  // w(i, j) is the total distance if we build a post office for x[i+1, j]
  auto w = [&](size_t i, size_t j) -> real {
    assert(i < j);
    i++;
    auto p = (i + j) / 2;
    auto left = x[p] * (p - i + 1) - (sum[p] - sum[i - 1]);
    auto right = sum[j] - sum[p] - x[p] * (j - p);
    return left + right;
  };

  parlay::sequence<real> E1, E2, E3, E4, E5;
  parlay::sequence<size_t> best;

  parlay::internal::timer tm;

  if (FLAGS_run.find("bf") != string::npos) {
    E1.resize(n + 1);
    BruteForceDP(n, E1, f, w);
    tm.next("brute-force");
  }

  if (FLAGS_run.find("seq") != string::npos) {
    E2.resize(n + 1);
    ConvexDPSequential(n, E2, f, w);
    tm.next("sequential");
  }

  if (FLAGS_run.find("par") != string::npos) {
    E3.resize(n + 1);
    best = ConvexDPParallel(n, E3, f, w);
    tm.next("parallel");
  }

  if (FLAGS_run.find("new1") != string::npos) {
    E4.resize(n + 1);
    ConvexDPNew(n, E4, f, w);
    tm.next("new2");
  }

  if (FLAGS_run.find("new2") != string::npos) {
    E4.resize(n + 1);
    ConvexDPNew2(n, E4, f, w);
    tm.next("new2");
  }

  size_t k = 0, t = n;
  while (t != 0) {
    k++;
    t = best[t];
  }
  cout << "\noutput size: " << k << endl;

  if (n <= 30) {
    std::cout << "best: ";
    for (size_t i = 1; i <= n; i++) std::cout << best[i] << ' ';
    std::cout << std::endl;
  }

  bool ok = parlay::all_of(parlay::iota(n + 1),
                           [&](size_t i) { return abs(E3[i] - E4[i]) < 1e-7; });
  cout << "\nok: " << ok << '\n';

  return 0;
}

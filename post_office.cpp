#include <iostream>
#include <random>

#include "brute_force_dp.h"
#include "config.h"
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
DEFINE_string(run, "seq,par", "bf, seq, par");

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

  parlay::sequence<real> E1(n + 1);
  parlay::sequence<real> E2(n + 1);
  parlay::sequence<real> E3(n + 1);
  parlay::sequence<size_t> best;

  parlay::internal::timer tm;

  if (FLAGS_run.find("bf") != string::npos) {
    BruteForceDP(n, E1, f, w);
    tm.next("brute-force");
  }

  if (FLAGS_run.find("seq") != string::npos) {
    ConvexDPSequential(n, E2, f, w);
    tm.next("sequential");
  }

  if (FLAGS_run.find("par") != string::npos) {
    best = ConvexDPParallel(n, E3, f, w);
    tm.next("parallel");
    size_t k = 0, t = n;
    while (t != 0) {
      k++;
      t = best[t];
    }
    cout << "output size: " << k << endl;
  }

  bool ok = parlay::all_of(parlay::iota(n + 1), [&](size_t i) {
    // return abs(E1[i] - E2[i]) < 1e-7 && abs(E2[i] - E3[i]) < 1e-7;
    return abs(E2[i] - E3[i]) < 1e-7;
  });
  cout << "ok: " << ok << '\n';

  // for (size_t i = 1; i <= n; i++) {
  //   if (abs(E2[i] - E3[i]) > 1e-7) {
  //     cout << i << ' ' << E2[i] << ' ' << E3[i] << endl;
  //   }
  // }

  return 0;
}

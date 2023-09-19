#include <iostream>
#include <random>

#include "brute_force_dp.h"
#include "config.h"
#include "convex_dp_parallel.h"
#include "convex_dp_sequential.h"
#include "gflags/gflags.h"
#include "parlay/internal/get_time.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"

using namespace std;

using uint64 = unsigned long long;

DEFINE_uint64(n, 10, "n");
DEFINE_uint64(range, 100, "range");
DEFINE_uint64(cost, 10, "cost");

mt19937 rng(0);
// mt19937 rng(chrono::high_resolution_clock::now().time_since_epoch().count());

auto MakeData(size_t n) {
  parlay::sequence<uint64> x(n + 1);
  for (size_t i = 1; i <= n; i++) {
    x[i] = rng() % FLAGS_range;
  }
  sort(x.begin() + 1, x.end());
  return x;
}

int main(int argc, char *argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto n = FLAGS_n;
  auto cost = FLAGS_cost;

  cout << "n = " << n << '\n';

  auto x = MakeData(n);
  // cout << "x: ";
  // for (int i = 1; i <= n; i++) cout << x[i] << " \n"[i == n];
  cout << "cost: " << cost << '\n';

  vector<size_t> sum(n + 1);
  for (size_t i = 1; i <= n; i++) {
    sum[i] = sum[i - 1] + x[i];
  }

  // D[i] = E[i] + cost
  auto f = [&](uint64 Ei) -> uint64 { return Ei + cost; };

  // E[j] = min(D[i] + w(i, j))
  // w(i, j) is the total distance if we build a post office for x[i+1, j]
  auto w = [&](size_t i, size_t j) -> uint64 {
    assert(i < j);
    i++;
    auto p = (i + j) / 2;
    auto left = x[p] * (p - i + 1) - (sum[p] - sum[i - 1]);
    auto right = sum[j] - sum[p] - x[p] * (j - p);
    return left + right;
  };

  parlay::sequence<uint64> E1(n + 1);
  parlay::sequence<uint64> E2(n + 1);
  parlay::sequence<uint64> E3(n + 1);

  parlay::internal::timer tm;
  BruteForceDP(n, E1, f, w);
  tm.next("brute-force");

  ConvexDPSequential(n, E2, f, w);
  tm.next("sequential");

  ConvexDPParallel(n, E3, f, w);
  tm.next("parallel");

  bool ok = parlay::all_of(parlay::iota(n + 1), [&](size_t i) {
    return E1[i] == E2[i] && E2[i] == E3[i];
  });
  cout << "ok: " << ok << '\n';

  return 0;
}

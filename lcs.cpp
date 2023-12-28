#include <iostream>
#include <random>

#include "gflags/gflags.h"
#include "lcs_brute_force.h"
#include "lcs_parallel.h"
#include "parlay/sequence.h"

using namespace std;

DEFINE_uint64(n, 10, "n");
DEFINE_uint64(m, 40, "# of arrows");
DEFINE_uint64(k, 5, "LCS");
DEFINE_string(run, "bf,par,seq", "bf, par, seq");

auto MakeData(size_t n, size_t m, size_t k) {
  assert(k <= m);
  assert(k <= n);
  assert(m <= k * n * 2 - k * k);
  parlay::sequence<parlay::sequence<size_t>> arrows(n + 1);
  parlay::parallel_for(1, k + 1, [&](size_t i) { arrows[i].push_back(i); });
  m -= k;
  auto Push = [&](size_t i, const auto& s) {
    if (m >= s.size()) {
      arrows[i].append(s);
      m -= s.size();
    } else {
      arrows[i].append(parlay::make_slice(s.begin(), s.begin() + m));
      m = 0;
    }
  };
  auto a = parlay::iota(n + 1);
  for (size_t i = 1; i <= n; i++) {
    if (m == 0) break;
    if (i <= k) {
      Push(i, parlay::make_slice(a.begin() + 1, a.begin() + i));
      parlay::sort_inplace(arrows[i]);
    } else {
      Push(i, parlay::make_slice(a.begin() + 1, a.begin() + k + 1));
    }
  }
  for (size_t i = 1; i <= n; i++) {
    if (m == 0) break;
    if (i <= k) {
      Push(i, parlay::make_slice(a.begin() + i + 1, a.end()));
    }
  }
  size_t tot = parlay::reduce(
      parlay::delayed_seq<size_t>(n + 1, [&](size_t i) -> size_t {
        if (i > 0) return arrows[i].size();
        else return 0;
      }));
  assert(tot == FLAGS_m);
  cout << "total arrows: " << tot << endl;
  return arrows;
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto n = FLAGS_n, m = FLAGS_m, k = FLAGS_k;
  cout << "\n-------------\nn: " << n << "\nm: " << m << "\nk: " << k << endl;
  auto arrows = MakeData(n, m, k);

  // for (int i = 1; i <= n; i++) {
  //   for (int x : arrows[i]) cout << x << ' ';
  //   cout << endl;
  // }

  parlay::internal::timer tm;
  size_t res1, res2;

  if (FLAGS_run.find("bf") != string::npos) {
    res1 = BruteForceLCS(n, arrows);
    std::cout << "bf res: " << res1 << std::endl;
    tm.next("brute force");
    assert(res1 == FLAGS_k);
  }

  if (FLAGS_run.find("par") != string::npos) {
    res2 = ParallelLCS(n, arrows);
    std::cout << "par res: " << res2 << std::endl;
    tm.next("parallel");
    assert(res2 == FLAGS_k);
  }

  if (FLAGS_run.find("seq") != string::npos) {
    res2 = ParallelLCS<true>(n, arrows);
    std::cout << "seq res: " << res2 << std::endl;
    tm.next("sequential");
    assert(res2 == FLAGS_k);
  }

  if (FLAGS_run.find("bf") != string::npos &&
      FLAGS_run.find("par") != string::npos)
    std::cout << "\nok: " << (res1 == res2) << std::endl;

  return 0;
}

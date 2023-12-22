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
DEFINE_string(run, "bf,par", "bf, par");

auto MakeData(size_t n, size_t m, size_t k) {
  assert(k < n);
  assert(m <= n * n);
  parlay::sequence<size_t> a(n + 1), b(n + 1);
  if (m > n - 2 * n * k + 2 * k * k) {
    for (size_t i = 0; i < n; i++) a[i] = b[i] = 0;
    return make_pair(a, b);
  }
  mt19937_64 rng(0);
  for (int i = 1; i <= k; i++) a[i] = b[i] = 0;
  for (int i = k + 1; i <= n; i++) a[i] = 1, b[i] = 2;
  size_t now = k * k, i = k + 1;
  while (now < m) {
    b[i++] = 1;
    now += n - k;
  }
  shuffle(a.begin() + 1, a.end(), rng);
  shuffle(b.begin() + 1, b.end(), rng);
  return make_pair(a, b);
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto n = FLAGS_n, m = FLAGS_m, k = FLAGS_k;
  auto [a, b] = MakeData(n, m, k);

  if (n <= 30) {
    cout << "a: " << endl;
    for (size_t i = 0; i < n; i++) cout << a[i] << " \n"[i == n - 1];
    cout << "b: " << endl;
    for (size_t i = 0; i < n; i++) cout << b[i] << " \n"[i == n - 1];
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

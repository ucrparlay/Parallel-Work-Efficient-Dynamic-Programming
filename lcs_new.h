
#include "parlay/primitives.h"

#define lc(x) ((x) << 1)
#define rc(x) ((x) << 1 | 1)

template <typename Seq>
size_t ParallelLCS(size_t n, const Seq& arrows) {
  auto b = parlay::scan_inclusive(
      parlay::delayed_seq<size_t>(n + 1, [&](size_t i) -> size_t {
        if (i > 0) return arrows[i].size();
        else return 0;
      }));
  size_t m = b[n];
  parlay::sequence<std::pair<size_t, size_t>> a(m + 1);
}
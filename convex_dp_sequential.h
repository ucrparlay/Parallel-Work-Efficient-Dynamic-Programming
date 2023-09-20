#ifndef CONVEX_DP_SEQUENTIAL_H_
#define CONVEX_DP_SEQUENTIAL_H_

#include <deque>
#include <type_traits>

#include "parlay/sequence.h"

template <typename Seq, typename F, typename W>
void ConvexDPSequential(size_t n, Seq& E, F f, W w) {
  using T = typename Seq::value_type;
  static_assert(std::is_same_v<T, std::invoke_result_t<W, size_t, size_t>>);
  static_assert(std::is_same_v<T, std::invoke_result_t<F, T>>);
  if (n >= 4) assert(w(1, 3) + w(2, 4) <= w(1, 4) + w(2, 3));

  auto Better = [&](size_t i1, size_t i2, size_t j) {
    auto t1 = f(E[i1]) + w(i1, j);
    auto t2 = f(E[i2]) + w(i2, j);
    return t1 < t2;
  };

  std::cout << "ConvexDPSequential start" << std::endl;

  std::deque<std::array<size_t, 3>> que = {{0, 1, n}};
  for (size_t j = 1; j <= n; j++) {
    while (que.front()[2] < j) que.pop_front();
    size_t i = que.front()[0];
    E[j] = f(E[i]) + w(i, j);
    if (que.front()[2] == j) que.pop_front();
    else que.front()[1] = j + 1;
    while (!que.empty()) {
      auto [i, l, r] = que.back();
      if (Better(j, i, l)) {
        que.pop_back();
      } else if (Better(j, i, r)) {
        size_t p = l, q = r, last = r;
        while (p <= q) {
          auto mid = (p + q) / 2;
          if (Better(j, i, mid)) {
            last = mid;
            q = mid - 1;
          } else {
            p = mid + 1;
          }
        }
        que.back()[2] = last - 1;
      } else {
        break;
      }
    }
    size_t t = que.empty() ? j + 1 : que.back()[2] + 1;
    if (t <= n) que.push_back({j, t, n});
  }
}

#endif  // CONVEX_DP_SEQUENTIAL_H_

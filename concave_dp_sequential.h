#ifndef CONCAVE_DP_SEQUENTIAL_H_
#define CONCAVE_DP_SEQUENTIAL_H_

#include <stack>
#include <type_traits>

#include "parlay/sequence.h"

template <typename Seq, typename F, typename W>
void ConcaveDPSequential(size_t n, Seq& E, F f, W w) {
  using T = typename Seq::value_type;
  static_assert(std::is_same_v<T, std::invoke_result_t<W, size_t, size_t>>);
  static_assert(std::is_same_v<T, std::invoke_result_t<F, T>>);
  if (n >= 4) assert(w(1, 3) + w(2, 4) >= w(1, 4) + w(2, 3));

  auto Better = [&](size_t i1, size_t i2, size_t j) {
    auto t1 = f(E[i1]) + w(i1, j);
    auto t2 = f(E[i2]) + w(i2, j);
    return t1 < t2;
  };

  std::stack<std::array<size_t, 3>> sta;
  sta.push({0, 1, n});
  for (size_t j = 1; j <= n; j++) {
    while (sta.top()[2] < j) sta.pop();
    size_t i = sta.top()[0];
    E[j] = f(E[i]) + w(i, j);
    if (sta.top()[2] == j) sta.pop();
    else sta.top()[1] = j + 1;
    while (!sta.empty()) {
      auto [i, l, r] = sta.top();
      if (Better(j, i, r)) {
        sta.pop();
      } else if (Better(j, i, l)) {
        size_t p = l, q = r, first = l;
        while (p <= q) {
          size_t mid = (p + q) / 2;
          if (Better(j, i, mid)) {
            first = mid;
            p = mid + 1;
          } else {
            q = mid - 1;
          }
        }
        sta.top()[1] = first + 1;
      } else {
        break;
      }
    }
    size_t t = sta.empty() ? n : sta.top()[1] - 1;
    if (t >= j + 1) sta.push({j, j + 1, t});
  }
}

#endif  // CONCAVE_DP_SEQUENTIAL_H_

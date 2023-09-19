#ifndef BRUTE_FORCE_DP_H_
#define BRUTE_FORCE_DP_H_

#include <type_traits>

#include "parlay/sequence.h"

template <typename Seq, typename F, typename W>
auto BruteForceDP(size_t n, Seq& E, F f, W w) {
  using T = typename Seq::value_type;
  static_assert(std::is_same_v<T, std::invoke_result_t<W, size_t, size_t>>);
  static_assert(std::is_same_v<T, std::invoke_result_t<F, T>>);

  auto D = [&](size_t i) { return f(E[i]); };

  for (size_t j = 1; j <= n; j++) {
    E[j] = std::numeric_limits<T>::max();
    for (size_t i = 0; i < j; i++) {
      E[j] = std::min(E[j], D(i) + w(i, j));
    }
  }

  return E;
}

#endif  // BRUTE_FORCE_DP_H_

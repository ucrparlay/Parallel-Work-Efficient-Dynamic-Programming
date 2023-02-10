#ifndef DC3_H_
#define DC3_H_

#include <iostream>
#include <tuple>

#include "debug.h"
#include "parlay/internal/get_time.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"
#include "suffix_array_parallel.h"

namespace {

// return (ranks, sa, lcp)
template <typename Seq>
parlay::sequence<unsigned int> DC3_internal_(const Seq& s, int dep = 0) {
  assert(*parlay::min_element(s) > 0);
  auto n = s.size();
  if (n <= 100000) {
    auto [rank, sa, lcp] = suffix_array_large_alphabet(s);
    return sa;
  }
  auto m = n + 3;
  while (m % 3) m++;
  auto ss = parlay::tabulate(m, [&](auto i) { return i < n ? s[i] : 0; });
  auto s12 = parlay::sequence<unsigned int>(m / 3 * 2, 0);
  parlay::parallel_for(0, m, [&](auto i) {
    if (i % 3) s12[(i / 3) * 2 + i % 3 - 1] = i;
  });
  parlay::stable_integer_sort_inplace(s12, [&](auto i) { return ss[i + 2]; });
  parlay::stable_integer_sort_inplace(s12, [&](auto i) { return ss[i + 1]; });
  parlay::stable_integer_sort_inplace(s12, [&](auto i) { return ss[i]; });
  auto same = parlay::tabulate(s12.size(), [&](auto i) {
    if (i == 0) return 1u;
    for (int k = 0; k < 3; k++) {
      if (ss[s12[i] + k] != ss[s12[i - 1] + k]) {
        return 1u;
      }
    }
    return 0u;
  });
  auto sum = parlay::scan_inclusive(same);
  auto tao = parlay::sequence<unsigned int>(m);
  parlay::parallel_for(0, m / 3 * 2, [&](auto i) { tao[s12[i]] = sum[i]; });
  parlay::parallel_for(0, m, [&](auto i) {
    if (i % 3 == 1) {
      s12[i / 3] = tao[i];
    } else if (i % 3 == 2) {
      s12[i / 3 + m / 3] = tao[i];
    }
  });
  auto sa12 = DC3_internal_(s12, dep + 1);  // recursive call
  parlay::parallel_for(0, sa12.size(), [&](auto i) {
    if (sa12[i] < m / 3) {
      sa12[i] = sa12[i] * 3 + 1;
    } else {
      sa12[i] = (sa12[i] - m / 3) * 3 + 2;
    }
  });
  auto& rank = tao;
  parlay::parallel_for(0, sa12.size(), [&](auto i) { rank[sa12[i]] = i; });
  auto sa0 = parlay::sequence<unsigned int>(m / 3);
  parlay::parallel_for(0, m / 3, [&](auto i) { sa0[i] = i * 3; });
  parlay::stable_integer_sort_inplace(sa0, [&](auto i) { return rank[i + 1]; });
  parlay::stable_integer_sort_inplace(sa0, [&](auto i) { return ss[i]; });
  parlay::parallel_for(0, m / 3, [&](auto i) { rank[sa0[i]] = i; });
  auto fff = [&](auto i, auto j) {
    assert(i % 3 == 0 && j % 3 > 0);
    if (j % 3 == 1) {
      if (ss[i] != ss[j]) return ss[i] < ss[j];
      return rank[i + 1] < rank[j + 1];
    } else {
      if (ss[i] != ss[j]) return ss[i] < ss[j];
      if (ss[i + 1] != ss[j + 1]) return ss[i + 1] < ss[j + 1];
      return rank[i + 2] < rank[j + 2];
    }
  };
  auto sa =
      parlay::merge(sa0, sa12, [&](auto i, auto j) { return !fff(j, i); });
  return parlay::filter(sa, [&](auto i) { return i < n; });
}

}  // namespace

template <typename Seq>
auto DC3(const Seq& s) {
  auto n = s.size();
  auto sa = DC3_internal_(s);
  auto rank = parlay::sequence<unsigned int>(n);
  parlay::parallel_for(0, n, [&](auto i) { rank[sa[i]] = i; });
  auto height = lcp(s, sa);
  auto lcp = parlay::sequence<unsigned int>(n);
  lcp[0] = 0;
  parlay::copy(height, parlay::make_slice(lcp.begin() + 1, lcp.end()));
  return std::make_tuple(rank, sa, lcp);
}

#endif  // DC3_H_
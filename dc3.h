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
parlay::sequence<unsigned int> DC3_internal_(const Seq& s) {
  auto n = s.size();
  assert(*parlay::min_element(s) > 0);
  assert(*parlay::max_element(s) <= n);
  if (n <= 100) {
    auto [rank, sa, lcp] = suffix_array_small_alphabet(s);
    return sa;
  }
  auto m = n;
  while (m % 3) m++;
  auto ss = parlay::tabulate(m + 3, [&](auto i) { return i < n ? s[i] : 0; });
  auto a12 = parlay::sequence<unsigned int>(m / 3 * 2, 0);
  parlay::parallel_for(0, m, [&](auto i) {
    if (i % 3) a12[(i / 3) * 2 + i % 3 - 1] = i;
  });
  parlay::stable_integer_sort_inplace(a12, [&](auto i) { return ss[i + 2]; });
  parlay::stable_integer_sort_inplace(a12, [&](auto i) { return ss[i + 1]; });
  parlay::stable_integer_sort_inplace(a12, [&](auto i) { return ss[i]; });
  auto same = parlay::tabulate(a12.size(), [&](auto i) -> unsigned int {
    if (i == 0) return 1u;
    auto p = a12[i], q = a12[i - 1];
    return ss[p] != ss[q] || ss[p + 1] != ss[q + 1] || ss[p + 2] != ss[q + 2];
  });
  auto sum = parlay::scan_inclusive(same);
  // if can not sort by 3 characters, call DC3 recursively
  if (sum.back() < sum.size()) {
    auto tao = parlay::sequence<unsigned int>(m);
    parlay::parallel_for(0, a12.size(), [&](auto i) { tao[a12[i]] = sum[i]; });
    parlay::parallel_for(0, m / 3, [&](auto i) {
      a12[i] = tao[i * 3 + 1];
      a12[i + m / 3] = tao[i * 3 + 2];
    });
    a12 = DC3_internal_(a12);
    parlay::parallel_for(0, a12.size(), [&](auto i) {
      if (a12[i] < m / 3) {
        a12[i] = a12[i] * 3 + 1;
      } else {
        a12[i] = (a12[i] - m / 3) * 3 + 2;
      }
    });
  }
  auto rank = parlay::sequence<unsigned int>(m);
  parlay::parallel_for(0, a12.size(), [&](auto i) { rank[a12[i]] = i; });
  auto a0 = parlay::sequence<unsigned int>(m / 3);
  parlay::parallel_for(0, m / 3, [&](auto i) { a0[i] = i * 3; });
  parlay::stable_integer_sort_inplace(a0, [&](auto i) { return rank[i + 1]; });
  parlay::stable_integer_sort_inplace(a0, [&](auto i) { return ss[i]; });
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
  auto a = parlay::merge(a0, a12, [&](auto i, auto j) { return !fff(j, i); });
  return parlay::filter(a, [&](auto i) { return i < n; });
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
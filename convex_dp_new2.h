#include <array>
#include <iostream>
#include <map>

#include "pam/pam.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"
#include "utils.h"

template <typename Seq, typename F, typename W>
auto ConvexDPNew2(size_t n, Seq& E, F f, W w) {
  std::cout << "\nConvexDPNew2 start" << std::endl;
  using T = typename Seq::value_type;
  static_assert(std::is_same_v<T, std::invoke_result_t<W, size_t, size_t>>);
  static_assert(std::is_same_v<T, std::invoke_result_t<F, T>>);
  if (n >= 4) assert(w(1, 3) + w(2, 4) <= w(1, 4) + w(2, 3));

  const int granularity = 256;

  auto Go = [&](size_t i, size_t j) { return f(E[i]) + w(i, j); };

  parlay::sequence<size_t> best(n + 1);
  parlay::sequence<std::array<size_t, 3>> intervals;
  const size_t inf = std::numeric_limits<size_t>::max();

  auto FindSentinel = [&](size_t i) -> size_t {
    assert(!intervals.empty());
    size_t l1, r1, l2, r2, m1, m2;
    l1 = 0, r1 = intervals.size() - 1;
    while (l1 <= r1) {
      m1 = (l1 + r1) / 2;
      if (intervals[m1][0] > i) {
        r1 = m1 - 1;
      } else if (intervals[m1][1] < i) {
        l1 = m1 + 1;
      } else {
        best[i] = intervals[m1][2];
        break;
      }
    }
    auto Ei = Go(best[i], i);
    E[i] = Ei;
    auto Check = [&](size_t i, size_t j, size_t bj) {
      return i < j && f(Ei) + w(i, j) < Go(bj, j);
    };
    l1 = 0, r1 = intervals.size() - 1;
    size_t sentinel = n + 1;
    while (l1 <= r1) {
      m1 = (l1 + r1) / 2;
      auto [a, b, c] = intervals[m1];
      if (Check(i, a, c)) {
        sentinel = a;
        r1 = m1 - 1;
      } else if (Check(i, b, c)) {
        l2 = a, r2 = b;
        while (l2 <= r2) {
          m2 = (l2 + r2) / 2;
          if (Check(i, m2, c)) {
            sentinel = m2;
            r2 = m2 - 1;
          } else {
            l2 = m2 + 1;
          }
        }
        break;
      } else {
        l1 = m1 + 1;
      }
    }
    return sentinel;
  };

  struct entry {
    using key_t = pair<size_t, size_t>;
    using val_t = size_t;
    static inline bool comp(key_t a, key_t b) { return a < b; }
  };

  using map_t = pam_map<entry>;
  using node_t = map_t::Tree::node;

  std::function<node_t*(size_t, size_t, size_t, size_t)> FindIntervals =
      [&](size_t jl, size_t jr, size_t il, size_t ir) -> node_t* {
    if (il > ir) return nullptr;
    if (jl == jr) {
      return map_t::Tree::join(nullptr, {{il, ir}, jl}, nullptr);
    }
    size_t im = (il + ir) / 2;
    auto a = parlay::delayed_seq<T>(jr - jl + 1, [&](size_t j) {
      j += jl;
      return Go(j, im);
    });
    size_t j0 = parlay::min_element(a) - a.begin() + jl;
    bool parallel = jr - jl > granularity || ir - il > granularity;
    node_t *lt, *rt;
    conditional_par_do(
        parallel, [&]() { lt = FindIntervals(jl, j0, il, im - 1); },
        [&]() { rt = FindIntervals(j0, jr, im + 1, ir); });
    return map_t::Tree::join(lt, {{im, im}, j0}, rt);
  };

  parlay::sequence<std::array<size_t, 3>> tmp(n + 1);
  parlay::sequence<pair<size_t, size_t>> rec(n + 1);

  auto GetNewIntervals = [&](size_t now, size_t to) {
    auto root = FindIntervals(now + 1, to, to + 1, n);
    size_t k = map_t::Tree::size(root);
    assert(k <= n);
    map_t::Tree::foreach_index(root, 0, [&](auto et, size_t i) {
      tmp[i][0] = et.first.first;
      tmp[i][1] = et.first.second;
      tmp[i][2] = et.second;
    });
    parlay::parallel_for(now + 1, to + 1,
                         [&](size_t i) { rec[i].first = rec[i].second = inf; });
    parlay::parallel_for(0, k, [&](size_t i) {
      if (i == 0 || tmp[i][2] != tmp[i - 1][2]) {
        rec[tmp[i][2]].first = tmp[i][0];
      }
      if (i == k - 1 || tmp[i][2] != tmp[i + 1][2]) {
        rec[tmp[i][2]].second = tmp[i][1];
      }
    });
    auto a =
        parlay::delayed_seq<T>(to - now, [&](size_t i) { return i + now + 1; });
    intervals = parlay::map(a, [&](size_t i) -> std::array<size_t, 3> {
      return {rec[i].first, rec[i].second, i};
    });
    intervals = parlay::filter(intervals, [&](auto& x) { return x[0] != inf; });
  };

  size_t now = 0;
  std::map<size_t, size_t> step;
  parlay::internal::timer t1("t1", false), t2("t2", false);
  intervals.push_back({1, n, 0});
  while (now < n) {
    t1.start();
    size_t s = 1;
    size_t nxt = n + 1;
    for (;;) {
      size_t l = now + (size_t(1) << (s - 1));
      size_t r = std::min(n, now + (size_t(1) << s) - 1);
      auto a = parlay::delayed_seq<size_t>(
          r - l + 1, [&](size_t i) { return FindSentinel(i + l); });
      nxt = std::min(nxt, *parlay::min_element(a));
      if (nxt <= r + 1) break;
      s++;
    }
    t1.stop();
    // std::cout << "now: " << now << ",  nxt: " << nxt << std::endl;
    size_t to = nxt - 1;
    step[to - now]++;
    if (nxt > n) break;
    t2.start();
    GetNewIntervals(now, to);
    now = to;
    t2.stop();
    // std::cout << "intervals: ";
    // for (auto [l, r, j] : intervals) {
    //   std::cout << "(" << l << "," << r << "," << j << ")";
    // }
    // std::cout << std::endl;
  }
  t1.total();
  t2.total();
  size_t step_sum = 0;
  for (auto [step, cnt] : step) {
    step_sum += cnt;
    // std::cout << "step: " << step << ", cnt: " << cnt << std::endl;
  }
  std::cout << "step_sum: " << step_sum << std::endl;
  // std::cout << "best: ";
  // for (size_t i = 1; i <= n; i++) std::cout << best[i] << ' ';
  // std::cout << std::endl;
  // std::cout << "E: ";
  // for (size_t i = 1; i <= n; i++) std::cout << E[i] << ' ';
  // std::cout << std::endl;
  std::cout << "ConvexDPNew2 end" << std::endl;
}

#ifndef LCS_PARALLEL_H_
#define LCS_PARALLEL_H_

#include "parlay/internal/group_by.h"
#include "parlay/parallel.h"
#include "parlay/primitives.h"
#include "utils.h"

#define lc(x) ((x) << 1)
#define rc(x) ((x) << 1 | 1)

template <typename Seq>
size_t ParallelLCS(const Seq& a, size_t n, const Seq& b, size_t m) {
  const size_t inf = std::numeric_limits<size_t>::max();
  const size_t granularity = 1 << 12;

  parlay::sequence<std::pair<size_t, size_t>> c(n + m);
  parlay::parallel_for(0, n, [&](size_t i) { c[i] = {a[i + 1], i + 1 + m}; });
  parlay::parallel_for(0, m, [&](size_t i) { c[i + n] = {b[i + 1], i + 1}; });
  auto cg = parlay::group_by_key(c);
  parlay::sequence<size_t> go(n + 1), now(n + 1);
  parlay::parallel_for(0, cg.size(), [&](size_t i) {
    auto& [key, vals] = cg[i];
    auto ais = parlay::filter(vals, [&](size_t x) { return x > m; });
    auto bis = parlay::filter(vals, [&](size_t x) { return x <= m; });
    parlay::parallel_for(0, ais.size(), [&](size_t j) {
      size_t id = ais[j] - m;
      go[id] = i;
    });
    vals.swap(bis);
  });
  std::cout << "a: ";
  for (size_t i = 1; i <= n; i++) std::cout << a[i] << ' ';
  std::cout << std::endl;
  std::cout << "b: ";
  for (size_t i = 1; i <= m; i++) std::cout << b[i] << ' ';
  std::cout << std::endl;
  std::cout << "cg: " << std::endl;
  for (auto& [key, vals] : cg) {
    std::cout << key << ", [";
    for (size_t i = 0; i < vals.size(); i++) {
      std::cout << vals[i];
      if (i != vals.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
  }
  std::cout << "go: ";
  for (size_t i = 1; i <= n; i++) std::cout << go[i] << ' ';
  std::cout << std::endl;

  auto Read = [&](size_t i) {
    if (now[i] >= cg[go[i]].second.size()) return inf;
    return cg[go[i]].second[now[i]];
  };

  parlay::sequence<size_t> tree(4 * n);

  std::function<void(size_t, size_t, size_t)> Construct =
      [&](size_t x, size_t l, size_t r) {
        if (l == r) {
          tree[x] = Read(l);
          return;
        }
        size_t mid = (l + r) / 2;
        bool parallel = r - l > granularity;
        conditional_par_do(
            parallel, [&]() { Construct(lc(x), l, mid); },
            [&]() { Construct(rc(x), mid + 1, r); });
        tree[x] = std::min(tree[lc(x)], tree[rc(x)]);
      };

  std::function<void(size_t, size_t, size_t, size_t, bool)> PrefixMin =
      [&](size_t x, size_t l, size_t r, size_t pre, bool is_first) {
        if (tree[x] > pre) return;
        if (l == r) {
          if (is_first) now[l] = cg[go[l]].second.size();
          else now[l]++;
          tree[x] = Read(l);
          return;
        }
        size_t mid = (l + r) / 2;
        if (tree[x] == tree[rc(x)]) {
          if (tree[lc(x)] <= pre && tree[lc(x)] < inf) {
            bool parallel = r - l > granularity;
            conditional_par_do(
                parallel, [&]() { PrefixMin(lc(x), l, mid, pre, is_first); },
                [&]() { PrefixMin(rc(x), mid + 1, r, tree[lc(x)], false); });
          } else {
            PrefixMin(rc(x), mid + 1, r, pre, is_first);
          }
        } else {
          PrefixMin(lc(x), l, mid, pre, is_first);
        }
        tree[x] = std::min(tree[lc(x)], tree[rc(x)]);
      };

  Construct(1, 1, n);
  size_t round = 0;
  while (tree[1] < inf) {
    round++;
    PrefixMin(1, 1, n, inf, true);
  }
  return round;
}

#endif  // LCS_PARALLEL_H_

#ifndef LCS_PARALLEL_H_
#define LCS_PARALLEL_H_

#include <set>

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
  size_t tot = parlay::reduce(
      parlay::delayed_seq<size_t>(n + 1, [&](size_t i) -> size_t {
        if (i > 0) return cg[go[i]].second.size();
        else return 0;
      }));
  std::cout << "tot arrows: " << tot << std::endl;

  // std::cout << "a: ";
  // for (size_t i = 1; i <= n; i++) std::cout << a[i] << ' ';
  // std::cout << std::endl;
  // std::cout << "b: ";
  // for (size_t i = 1; i <= m; i++) std::cout << b[i] << ' ';
  // std::cout << std::endl;
  // std::cout << "cg: " << std::endl;
  // for (auto& [key, vals] : cg) {
  //   std::cout << key << ", [";
  //   for (size_t i = 0; i < vals.size(); i++) {
  //     std::cout << vals[i];
  //     if (i != vals.size() - 1) std::cout << ", ";
  //   }
  //   std::cout << "]" << std::endl;
  // }
  // std::cout << "go: ";
  // for (size_t i = 1; i <= n; i++) std::cout << go[i] << ' ';
  // std::cout << std::endl;

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

  std::function<void(size_t, size_t, size_t, size_t)> PrefixMin =
      [&](size_t x, size_t l, size_t r, size_t pre) {
        if (tree[x] > pre) return;
        if (l == r) {
          while (now[l] < cg[go[l]].second.size() &&
                 cg[go[l]].second[now[l]] <= pre) {
            now[l]++;
          }
          tree[x] = Read(l);
          return;
        }
        size_t mid = (l + r) / 2;
        if (tree[x] == tree[rc(x)]) {
          if (tree[lc(x)] <= pre && tree[lc(x)] < inf) {
            bool parallel = r - l > granularity;
            size_t lc_val = tree[lc(x)];
            conditional_par_do(
                parallel, [&]() { PrefixMin(lc(x), l, mid, pre); },
                [&]() { PrefixMin(rc(x), mid + 1, r, lc_val); });
          } else {
            PrefixMin(rc(x), mid + 1, r, pre);
          }
        } else {
          PrefixMin(lc(x), l, mid, pre);
        }
        tree[x] = std::min(tree[lc(x)], tree[rc(x)]);
      };

  Construct(1, 1, n);
  size_t round = 0;
  while (tree[1] < inf) {
    round++;
    PrefixMin(1, 1, n, inf);
    // std::cout << "now: ";
    // for (int i = 1; i <= n; i++) std::cout << now[i] << ' ';
    // std::cout << std::endl;
  }
  return round;
}

#endif  // LCS_PARALLEL_H_

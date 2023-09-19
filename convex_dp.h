#ifndef CONVEX_DP_H_
#define CONVEX_DP_H_

#include <algorithm>
#include <map>
#include <type_traits>

#include "parlay/primitives.h"
#include "parlay/sequence.h"

struct BST {
  size_t n;
  parlay::sequence<size_t> best, tag;

  BST(size_t n_) : n(n_) {
    best.assign(n, 0);
    tag.assign(n, 0);
  }

  void Pushdown(size_t x, size_t tl, size_t tr) {
    if (tag[x] == 0) return;
    if (x > tl) {
      size_t lc = (tl + x - 1) / 2;
      best[lc] = tag[lc] = tag[x];
    }
    if (x < tr) {
      size_t rc = (x + 1 + tr) / 2;
      best[rc] = tag[rc] = tag[x];
    }
    tag[x] = 0;
  }

  size_t GetBest(size_t tl, size_t tr, size_t i) {
    size_t x = (tl + tr) / 2;
    Pushdown(x, tl, tr);
    if (x == i) return best[x];
    if (i < x) return GetBest(tl, x - 1, i);
    else return GetBest(x + 1, tr, i);
  }
};

template <typename Seq, typename F, typename W>
void ConvexDP(size_t n, Seq& E, F f, W w) {
  using T = typename Seq::value_type;
  static_assert(std::is_same_v<T, std::invoke_result_t<W, size_t, size_t>>);
  static_assert(std::is_same_v<T, std::invoke_result_t<F, T>>);

  BST bst(n + 1);

  std::function<bool(size_t, size_t, size_t, size_t, size_t, size_t)>
      HaveDependency = [&](size_t tl, size_t tr, size_t pl, size_t pr,
                           size_t to, size_t Eto) {
        if (tl > tr) return false;
        if (tr < pl || tl > pr) return false;
        size_t x = (tl + tr) / 2;
        bst.Pushdown(x, tl, tr);
        if (x < pl) {
          return HaveDependency(x + 1, tr, pl, pr, to, Eto);
        } else if (x > pr) {
          return HaveDependency(tl, x - 1, pl, pr, to, Eto);
        } else {
          size_t bx = bst.best[x];
          size_t Ex = f(E[bx]) + w(bx, x);
          if (f(Ex) + w(x, to) < Eto) return true;
          bool resl = false, resr = false;
          parlay::parallel_do(
              [&]() { resl = HaveDependency(tl, x - 1, pl, pr, to, Eto); },
              [&]() { resr = HaveDependency(x + 1, tr, pl, pr, to, Eto); });
          return resl || resr;
        }
      };

  std::function<void(size_t, size_t, size_t, size_t, size_t, size_t)> Update =
      [&](size_t tl, size_t tr, size_t bl, size_t br, size_t pl, size_t pr) {
        if (tl > tr) return;
        if (tr < pl || tl > pr) return;
        size_t x = (tl + tr) / 2;
        bst.Pushdown(x, tl, tr);
        if (x < pl) {
          Update(x + 1, tr, bl, br, pl, pr);
        } else if (x > pr) {
          Update(tl, x - 1, bl, br, pl, pr);
        } else if (bl == br) {
          size_t bx = bst.best[x];
          if (f(E[bl]) + w(bl, x) < f(E[bx]) + w(bx, x)) {
            bst.best[x] = bl;
            if (x < tr) {
              size_t y = (x + 1 + tr) / 2;
              bst.best[y] = bst.tag[y] = bl;
            }
            Update(tl, x - 1, bl, br, pl, pr);
          } else {
            Update(x + 1, tr, bl, br, pl, pr);
          }
        } else {
          auto a = parlay::iota(br - bl + 1);
          auto it = parlay::min_element(a, [&](size_t i, size_t j) {
            i += bl, j += bl;
            return f(E[i]) + w(i, x) < f(E[j]) + w(j, x);
          });
          auto i = it - a.begin() + bl;
          size_t bx = bst.best[x];
          if (f(E[i]) + w(i, x) < f(E[bx]) + w(bx, x)) {
            bst.best[x] = i;
            parlay::parallel_do([&]() { Update(tl, x - 1, bl, i, pl, pr); },
                                [&]() { Update(x + 1, tr, i, br, pl, pr); });
          } else {
            Update(x + 1, tr, bl, br, pl, pr);
          }
        }
      };

  size_t now = 0;
  std::map<size_t, size_t> step;
  while (now < n) {
    size_t to = now + 1;
    while (to < n) {
      size_t nxt = std::min(n, now + 2 * (to - now));
      size_t bnxt = bst.GetBest(1, n, nxt);
      auto Enxt = f(E[bnxt]) + w(bnxt, nxt);
      if (HaveDependency(1, n, now + 1, nxt - 1, nxt, Enxt)) break;
      else to = nxt;
    }
    step[to - now]++;
    parlay::parallel_for(now + 1, to + 1, [&](size_t j) {
      size_t i = bst.best[j];
      assert(i <= now);
      E[j] = f(E[i]) + w(i, j);
    });
    Update(1, n, now + 1, to, to + 1, n);
    now = to;
  }
  for (auto [step, cnt] : step) {
    std::cout << "step: " << step << ", cnt: " << cnt << std::endl;
  }
}

#endif  // CONVEX_DP_H_

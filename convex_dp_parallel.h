#ifndef CONVEX_DP_PARALLEL_H_
#define CONVEX_DP_PARALLEL_H_

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
};

template <typename Seq, typename F, typename W>
void ConvexDPParallel(size_t n, Seq& E, F f, W w) {
  std::cout << "ConvexDPParallel start" << std::endl;
  using T = typename Seq::value_type;
  static_assert(std::is_same_v<T, std::invoke_result_t<W, size_t, size_t>>);
  static_assert(std::is_same_v<T, std::invoke_result_t<F, T>>);
  if (n >= 4) assert(w(1, 3) + w(2, 4) <= w(1, 4) + w(2, 3));

  BST bst(n + 1);

  std::function<void(size_t, size_t, size_t, size_t)> Visit =
      [&](size_t tl, size_t tr, size_t pl, size_t pr) {
        if (tl > tr) return;
        if (tr < pl || tl > pr) return;
        size_t x = (tl + tr) / 2;
        bst.Pushdown(x, tl, tr);
        if (x < pl) {
          Visit(x + 1, tr, pl, pr);
        } else if (x > pr) {
          Visit(tl, x - 1, pl, pr);
        } else {
          parlay::parallel_do([&]() { Visit(tl, x - 1, pl, pr); },
                              [&]() { Visit(x + 1, tr, pl, pr); });
        }
      };

  parlay::internal::timer tvv("tvv", false);
  auto HaveDependency = [&](size_t now, size_t lst, size_t nxt) {
    tvv.start();
    Visit(1, n, lst, nxt);
    tvv.stop();
    size_t bnxt = bst.best[nxt];
    auto Enxt = f(E[bnxt]) + w(bnxt, nxt);
    return parlay::any_of(parlay::iota(nxt - now - 1), [&](size_t i) {
      i += now + 1;
      size_t bi = bst.best[i];
      auto Ei = f(E[bi]) + w(bi, i);
      return f(Ei) + w(i, nxt) < Enxt;
    });
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
  parlay::internal::timer t1("t1", false);
  parlay::internal::timer t2("t2", false);
  while (now < n) {
    if (now % 100000 == 0) std::cout << now << std::endl;
    t1.start();
    size_t to = now + 1;
    while (to < n) {
      size_t nxt = std::min(n, now + 2 * (to - now));
      if (HaveDependency(now, to, nxt)) break;
      else to = nxt;
    }
    t1.stop();
    step[to - now]++;
    t2.start();
    parlay::parallel_for(now + 1, to + 1, [&](size_t j) {
      size_t i = bst.best[j];
      E[j] = f(E[i]) + w(i, j);
    });
    Update(1, n, now + 1, to, to + 1, n);
    t2.stop();
    now = to;
  }
  for (auto [step, cnt] : step) {
    std::cout << "step: " << step << ", cnt: " << cnt << std::endl;
  }
  t1.total();
  t2.total();
  tvv.total();
  std::cout << "ConvexDPParallel end" << std::endl;
}

#endif  // CONVEX_DP_PARALLEL_H_

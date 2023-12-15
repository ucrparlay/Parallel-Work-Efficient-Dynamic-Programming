#ifndef CONVEX_DP_PARALLEL_H_
#define CONVEX_DP_PARALLEL_H_

#include <algorithm>
#include <iostream>
#include <map>
#include <type_traits>

#include "parlay/primitives.h"
#include "parlay/sequence.h"

template <typename Seq, typename F, typename W>
auto ConvexDPParallel(size_t n, Seq& E, F f, W w) {
  std::cout << "\nConvexDPParallel start" << std::endl;
  using T = typename Seq::value_type;
  static_assert(std::is_same_v<T, std::invoke_result_t<W, size_t, size_t>>);
  static_assert(std::is_same_v<T, std::invoke_result_t<F, T>>);
  if (n >= 4) assert(w(1, 3) + w(2, 4) <= w(1, 4) + w(2, 3));

  auto Go = [&](size_t i, size_t j) { return f(E[i]) + w(i, j); };

  parlay::sequence<size_t> best(n + 1), tag(n + 1);

  auto Pushdown = [&](size_t x, size_t tl, size_t tr) {
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
  };

  const size_t granuality = 256;

  std::function<void(size_t, size_t, size_t, size_t)> Visit =
      [&](size_t tl, size_t tr, size_t pl, size_t pr) {
        if (tl > tr) return;
        if (tr < pl || tl > pr) return;
        size_t x = (tl + tr) / 2;
        Pushdown(x, tl, tr);
        if (x < pl) {
          Visit(x + 1, tr, pl, pr);
        } else if (x > pr) {
          Visit(tl, x - 1, pl, pr);
        } else {
          E[x] = Go(best[x], x);
          if (pr - pl > granuality) {
            parlay::parallel_do([&]() { Visit(tl, x - 1, pl, pr); },
                                [&]() { Visit(x + 1, tr, pl, pr); });
          } else {
            Visit(tl, x - 1, pl, pr);
            Visit(x + 1, tr, pl, pr);
          }
        }
      };

  auto HaveDependency = [&](size_t now, size_t nxt) {
    return parlay::any_of(parlay::iota(nxt - now - 1), [&](size_t i) {
      i += now + 1;
      return Go(i, nxt) < E[nxt];
    });
  };

  std::function<void(size_t, size_t, size_t, size_t, size_t, size_t)> Update =
      [&](size_t tl, size_t tr, size_t bl, size_t br, size_t pl, size_t pr) {
        if (tl > tr) return;
        if (tr < pl || tl > pr) return;
        size_t x = (tl + tr) / 2;
        Pushdown(x, tl, tr);
        if (x < pl) {
          Update(x + 1, tr, bl, br, pl, pr);
        } else if (x > pr) {
          Update(tl, x - 1, bl, br, pl, pr);
        } else if (bl == br) {
          if (Go(bl, x) < Go(best[x], x)) {
            best[x] = bl;
            if (x < tr) {
              size_t y = (x + 1 + tr) / 2;
              best[y] = tag[y] = bl;
            }
            Update(tl, x - 1, bl, br, pl, pr);
          } else {
            Update(x + 1, tr, bl, br, pl, pr);
          }
        } else {
          auto a = parlay::iota(br - bl + 1);
          auto it = parlay::min_element(a, [&](size_t i, size_t j) {
            i += bl, j += bl;
            return Go(i, x) < Go(j, x);
          });
          auto i = it - a.begin() + bl;
          if (Go(i, x) < Go(best[x], x)) {
            best[x] = i;
            if (br - bl > granuality && pr - pl > granuality) {
              parlay::parallel_do([&]() { Update(tl, x - 1, bl, i, pl, pr); },
                                  [&]() { Update(x + 1, tr, i, br, pl, pr); });
            } else {
              Update(tl, x - 1, bl, i, pl, pr);
              Update(x + 1, tr, i, br, pl, pr);
            }
          } else {
            Update(x + 1, tr, bl, br, pl, pr);
          }
        }
      };

  size_t now = 0;
  std::map<size_t, size_t> step;
  parlay::internal::timer t1("t1", false), t2("t2", false);
  while (now < n) {
    t1.start();
    size_t to = now;
    while (to < n) {
      size_t s = std::max(2 * (to - now), size_t(1));
      size_t nxt = std::min(n, now + s);
      Visit(1, n, to + 1, nxt);
      if (HaveDependency(now, nxt)) break;
      else to = nxt;
    }
    t1.stop();
    t2.start();
    step[to - now]++;
    Update(1, n, now + 1, to, to + 1, n);
    // std::cout << "now: " << now << ", to: " << to << std::endl;
    now = to;
    t2.stop();
  }
  t1.total();
  t2.total();
  size_t step_sum = 0;
  for (auto [step, cnt] : step) {
    step_sum += cnt;
    std::cout << "step: " << step << ", cnt: " << cnt << std::endl;
  }
  std::cout << "step_sum: " << step_sum << std::endl;
  // std::cout << "E: ";
  // for (size_t i = 1; i <= n; i++) std::cout << E[i] << ' ';
  // std::cout << std::endl;
  std::cout << "ConvexDPParallel end" << std::endl;
  return best;
}

#endif  // CONVEX_DP_PARALLEL_H_

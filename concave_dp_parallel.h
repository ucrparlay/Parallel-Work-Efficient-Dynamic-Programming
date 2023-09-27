#ifndef CONCAVE_DP_PARALLEL_H_
#define CONCAVE_DP_PARALLEL_H_

#include <algorithm>
#include <map>
#include <type_traits>

#include "parlay/internal/get_time.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"

template <typename Seq, typename F, typename W>
auto ConcaveDPParallel(size_t n, Seq& E, F f, W w) {
  std::cout << "ConcaveDPParallel start" << std::endl;
  using T = typename Seq::value_type;
  static_assert(std::is_same_v<T, std::invoke_result_t<W, size_t, size_t>>);
  static_assert(std::is_same_v<T, std::invoke_result_t<F, T>>);
  if (n >= 4) assert(w(1, 3) + w(2, 4) >= w(1, 4) + w(2, 3));

  auto Go = [&](size_t i, size_t j) { return f(E[i]) + w(i, j); };

  parlay::sequence<size_t> best(n + 1), tag(n + 1);
  parlay::sequence<size_t> best2(n + 1);

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
          best2[x] = best[x];
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
      return Go(i, i + 1) < E[i + 1];
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
            if (x > tl) {
              size_t y = (tl + x - 1) / 2;
              best[y] = tag[y] = bl;
            }
            Update(x + 1, tr, bl, br, pl, pr);
          } else {
            Update(tl, x - 1, bl, br, pl, pr);
          }
        } else {
          auto a = parlay::iota(br - bl + 1);
          auto it = parlay::min_element(a, [&](size_t i, size_t j) {
            i += bl, j += bl;
            return Go(i, x) < Go(j, x);
          });
          auto i = it - a.begin() + bl;
          if (Go(i, x) < E[x]) {
            best[x] = i;
            if (br - bl > granuality && pr - pl > granuality) {
              parlay::parallel_do([&]() { Update(tl, x - 1, i, br, pl, pr); },
                                  [&]() { Update(x + 1, tr, bl, i, pl, pr); });
            } else {
              Update(tl, x - 1, i, br, pl, pr);
              Update(x + 1, tr, bl, i, pl, pr);
            }
          } else {
            Update(tl, x - 1, bl, br, pl, pr);
          }
        }
      };

  size_t now = 0;
  std::map<size_t, size_t> step;
  while (now < n) {
    size_t to = now;
    while (to < n) {
      size_t s = std::max(2 * (to - now), size_t(1));
      size_t nxt = std::min(n, now + s);
      Visit(1, n, to + 1, nxt);
      if (HaveDependency(now, nxt)) break;
      else to = nxt;
    }
    step[to - now]++;
    Update(1, n, now + 1, to, to + 1, n);
    now = to;
  }
  for (auto [step, cnt] : step) {
    std::cout << "step: " << step << ", cnt: " << cnt << std::endl;
  }
  std::cout << "ConcaveDPParallel end" << std::endl;
  return best2;
}

#endif  // CONCAVE_DP_PARALLEL_H_

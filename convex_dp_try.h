#include <iostream>
#include <map>

#include "parlay/primitives.h"
#include "parlay/sequence.h"

template <typename Seq, typename F, typename W>
auto ConvexDPTry(size_t n, Seq& E, F f, W w) {
  std::cout << "ConvexDPTry start" << std::endl;
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

  auto Search = [&](size_t i) {
    size_t l = i + 1, r = n, res = n + 1;
    while (l <= r) {
      size_t mid = (l + r) / 2;
      if (Go(i, mid) < E[mid]) {
        res = mid;
        r = mid - 1;
      } else {
        l = mid + 1;
      }
    }
    return res;
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
  while (now < n) {
    Visit(1, n, now + 1, n);
    size_t to = n;
    for (size_t i = now + 1; i <= n; i++) {
      size_t j = Search(i);
      to = std::min(to, j - 1);
    }
    step[to - now]++;
    Update(1, n, now + 1, to, to + 1, n);
    // std::cout << "now: " << now << ", to: " << to << std::endl;
    now = to;
  }
  Visit(1, n, 1, n);
  size_t step_sum = 0;
  for (auto [step, cnt] : step) {
    step_sum += cnt;
    std::cout << "step: " << step << ", cnt: " << cnt << std::endl;
  }
  std::cout << "step_sum: " << step_sum << std::endl;
  std::cout << "ConvexDPTry end" << std::endl;
  return best;
}
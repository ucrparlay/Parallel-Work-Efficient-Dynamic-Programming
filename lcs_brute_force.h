#ifndef LCS_BRUTE_FORCE_H_
#define LCS_BRUTE_FORCE_H_

#include <vector>

template <typename Seq>
size_t BruteForceLCS(size_t n, const Seq& arrows) {
  std::vector a(n + 1, std::vector<bool>(n + 1));
  for (size_t i = 1; i <= n; i++) {
    for (auto x : arrows[i]) a[i][x] = true;
  }
  std::vector dp(n + 1, std::vector<size_t>(n + 1));
  for (size_t i = 1; i <= n; i++) {
    for (size_t j = 1; j <= n; j++) {
      dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
      if (a[i][j]) {
        dp[i][j] = std::max(dp[i][j], dp[i - 1][j - 1] + 1);
      }
    }
  }
  return dp[n][n];
}

#endif  // LCS_BRUTE_FORCE_H_

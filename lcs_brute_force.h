#ifndef LCS_BRUTE_FORCE_H_
#define LCS_BRUTE_FORCE_H_

#include <vector>

template <typename Seq>
size_t BruteForceLCS(const Seq& a, size_t n, const Seq& b, size_t m) {
  std::vector dp(n + 1, std::vector<size_t>(m + 1));
  for (size_t i = 1; i <= n; i++) {
    for (size_t j = 1; j <= m; j++) {
      dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
      if (a[i] == b[j]) {
        dp[i][j] = std::max(dp[i][j], dp[i - 1][j - 1] + 1);
      }
    }
  }
  return dp[n][m];
}

#endif  // LCS_BRUTE_FORCE_H_

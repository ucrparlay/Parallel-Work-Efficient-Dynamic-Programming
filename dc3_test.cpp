#include "dc3.h"

#include <iostream>
#include <random>

#include "dc3_config.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "parlay/internal/get_time.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"

using namespace std;

bool TestDC3(int n, int alpha) {
  LOG(INFO) << "n: " << n << endl;
  std::mt19937 rng(0);
  auto a = parlay::sequence<unsigned int>(n);
  for (int i = 0; i < n; i++) {
    a[i] = rng() % alpha + 1;
  }
  double x = 0, y = 0;
  for (int rr = 0; rr < 5; rr++) {
    LOG(INFO) << "round: " << rr << endl;
    parlay::internal::timer tt;
    auto sa1 = suffix_array(a);
    x += tt.stop();
    tt.start();
    auto sa2 = DC3(a);
    y += tt.stop();
    if (rr > 0) continue;
    for (int i = 0; i < n; i++) {
      if (sa1[i] != sa2[i]) return false;
    }
  }
  x /= 5, y /= 5;
  LOG(INFO) << "parlay SA: " << x << endl;
  LOG(INFO) << "DC3: " << y << endl;
  LOG(INFO) << "rate: " << y / x << endl;
  return true;
}

DEFINE_int32(n, 10000, "n");
DEFINE_validator(n, [](const char* flagname, int n) { return n > 0; });

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  FLAGS_log_dir = CMAKE_CURRENT_SOURCE_DIR "/logs";
  FLAGS_alsologtostderr = 1;
  google::InitGoogleLogging(argv[0]);
  CHECK(TestDC3(FLAGS_n, 128));
  LOG(INFO) << "Test Pass!" << endl;
  return 0;
}
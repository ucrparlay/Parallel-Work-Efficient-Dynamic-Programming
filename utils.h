#ifndef UTILS_H_
#define UTILS_H_

#include "parlay/parallel.h"

template <typename Left, typename Right>
void conditional_par_do(bool parallel, Left left, Right right) {
  if (parallel) {
    parlay::parallel_do(left, right);
  } else {
    left();
    right();
  }
}

#endif  // UTILS_H_

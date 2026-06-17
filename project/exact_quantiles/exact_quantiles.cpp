#include "exact_quantiles.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <limits>
#include <vector>

// Parallel std::sort needs both <execution> and a library that actually
// implements the parallel algorithms (libstdc++ + TBB). Apple's libc++ ships
// neither, so fall back to a serial std::sort there instead of failing to build.
#if defined(__has_include)
#  if __has_include(<execution>) && defined(__cpp_lib_parallel_algorithm)
#    include <execution>
#    define EQ_HAVE_PARALLEL_SORT 1
#  endif
#endif

using namespace std;
ExactQuantiles::ExactQuantiles() {
}

void ExactQuantiles::add(double val) {
  val_vec_.push_back(val);
  sorted_ = false;
}

void ExactQuantiles::sort() {

#ifdef EQ_HAVE_PARALLEL_SORT
  // uses intel thread building blocks as the parallel backend
  std::sort(std::execution::par_unseq, val_vec_.begin(), val_vec_.end());
#else
  std::sort(val_vec_.begin(), val_vec_.end());
#endif

  //set sort via bool, we assume that a single add makes vec unsorted
  sorted_ = true;
}

double ExactQuantiles::get_quantile_value(double quantile) {
  if (quantile < 0.0 || quantile > 1) {
    std::cout << "FAIL: ExactQuantiles.get_quantile_value() called with invalid quantile < 0 or 1 < quantile:" << quantile << std::endl;
    exit(1);
  }

  if (val_vec_.empty()) return std::numeric_limits<double>::quiet_NaN();

  if(!sorted_) {
    cout << "WARNING slow lookup, needs to sort first" <<  endl;
    sort();
  }

  // Use the same rank convention as DDsketch (rank = q * (N - 1)) so the exact
  // baseline reports the value the sketch is approximating - a fair comparison.
  size_t idx = static_cast<size_t>(quantile * (val_vec_.size() - 1));
  if (idx >= val_vec_.size()) idx = val_vec_.size() - 1;  // guard q == 1.0
  return val_vec_[idx];
}

size_t ExactQuantiles::total_arr_size() { return val_vec_.size() ; }

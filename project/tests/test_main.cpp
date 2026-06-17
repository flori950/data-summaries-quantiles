// Lightweight assertion-based tests for the quantile data structures.
// Returns the number of failed checks (0 = success), so it plugs into ctest.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

#include "ddsketch/ddsketch.h"
#include "exact_quantiles/exact_quantiles.h"
#include "histogram_sketch/histogram_sketch.h"
#include "tdigest/tdigest.h"
#include "kll/kll.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                              \
  do {                                                               \
    if (!(cond)) {                                                   \
      std::printf("  FAIL: %s  (%s:%d)\n", (msg), __FILE__, __LINE__); \
      g_failures++;                                                  \
    }                                                                \
  } while (0)

static bool close_rel(double a, double b, double rel) {
  double denom = std::max(1.0, std::fabs(b));
  return std::fabs(a - b) <= rel * denom;
}

// reproducible uniform[-1,1] data
static std::vector<double> uniform_data(size_t n) {
  std::default_random_engine eng;
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  std::vector<double> v(n);
  for (auto& x : v) x = u(eng);
  return v;
}

static const std::vector<double> QS = {0.5, 0.75, 0.9, 0.95, 0.99, 1.0};

static void test_exact_quantiles_basic() {
  std::printf("test_exact_quantiles_basic\n");
  ExactQuantiles eq;
  for (double v : {5.0, 1.0, 4.0, 2.0, 3.0}) eq.add(v);  // unsorted on purpose
  eq.sort();
  // rank = q*(N-1): q=0 -> idx0 (1.0), q=0.5 -> idx2 (3.0), q=1 -> idx4 (5.0)
  CHECK(eq.get_quantile_value(0.0) == 1.0, "eq q0 == min");
  CHECK(eq.get_quantile_value(0.5) == 3.0, "eq median");
  CHECK(eq.get_quantile_value(1.0) == 5.0, "eq q1 == max");

  ExactQuantiles empty;
  CHECK(std::isnan(empty.get_quantile_value(0.5)), "eq empty -> NaN");
}

static void test_ddsketch_accuracy_vs_exact() {
  std::printf("test_ddsketch_accuracy_vs_exact\n");
  auto data = uniform_data(200000);
  DDsketch sk(0.01);
  ExactQuantiles eq;
  for (double v : data) { sk.add(v); eq.add(v); }
  eq.sort();
  for (double q : QS) {
    double dd = sk.get_quantile_value(q);
    double ex = eq.get_quantile_value(q);
    // DDsketch guarantees ~1% relative accuracy; allow a small margin.
    CHECK(close_rel(dd, ex, 0.02), "ddsketch within 2% of exact");
  }
  CHECK(sk.num_values() == (double)data.size(), "ddsketch count");
}

static void test_ddsketch_merge_equals_single() {
  std::printf("test_ddsketch_merge_equals_single\n");
  auto data = uniform_data(120000);
  DDsketch single;
  for (double v : data) single.add(v);

  // split across 4 partitions, fill independently, then merge
  std::vector<DDsketch> parts(4);
  for (size_t i = 0; i < data.size(); i++) parts[i % 4].add(data[i]);
  for (int i = 1; i < 4; i++) parts[0].merge(parts[i]);

  for (double q : QS) {
    CHECK(parts[0].get_quantile_value(q) == single.get_quantile_value(q),
          "merged ddsketch == single-pass ddsketch");
  }
  CHECK(parts[0].num_values() == single.num_values(), "merged count matches");
}

static void test_ddsketch_negative_values() {
  std::printf("test_ddsketch_negative_values\n");
  DDsketch sk;
  ExactQuantiles eq;
  std::default_random_engine eng;
  std::normal_distribution<double> nd(0.0, 5.0);
  for (int i = 0; i < 100000; i++) { double v = nd(eng); sk.add(v); eq.add(v); }
  eq.sort();
  for (double q : {0.1, 0.25, 0.5, 0.75, 0.9}) {
    CHECK(close_rel(sk.get_quantile_value(q), eq.get_quantile_value(q), 0.03),
          "ddsketch handles negatives within tolerance");
  }
}

static void test_histogram_basic_and_merge() {
  std::printf("test_histogram_basic_and_merge\n");
  auto data = uniform_data(200000);
  HistogramSketch single;            // [-50,50], 100k bins -> width 1e-3
  HistogramSketch a, b;
  ExactQuantiles eq;
  for (size_t i = 0; i < data.size(); i++) {
    single.add(data[i]);
    (i % 2 ? a : b).add(data[i]);
    eq.add(data[i]);
  }
  eq.sort();
  for (double q : QS) {
    // absolute error bounded by bin width (1e-3); compare loosely
    CHECK(std::fabs(single.get_quantile_value(q) - eq.get_quantile_value(q)) < 0.01,
          "histogram within bin width of exact");
  }
  a.merge(b);
  for (double q : QS) {
    CHECK(a.get_quantile_value(q) == single.get_quantile_value(q),
          "merged histogram == single-pass histogram");
  }
  CHECK(single.out_of_range() == 0, "no out-of-range for uniform[-1,1]");
}

static void test_histogram_out_of_range() {
  std::printf("test_histogram_out_of_range\n");
  HistogramSketch h(0.0, 1.0, 100);
  h.add(-5.0);  // below range
  h.add(7.0);   // above range
  h.add(0.5);   // in range
  CHECK(h.out_of_range() == 2, "counts out-of-range values");
  CHECK(h.num_values() == 3.0, "still counts all values");
}

static void test_tdigest_accuracy_and_merge() {
  std::printf("test_tdigest_accuracy_and_merge\n");
  auto data = uniform_data(200000);
  TDigest td(100.0);
  ExactQuantiles eq;
  for (double v : data) { td.add(v); eq.add(v); }
  eq.sort();
  for (double q : {0.01, 0.1, 0.5, 0.9, 0.99}) {
    // t-digest is value-accurate; allow a small absolute margin on uniform[-1,1]
    CHECK(std::fabs(td.get_quantile_value(q) - eq.get_quantile_value(q)) < 0.02,
          "tdigest within 0.02 of exact on uniform");
  }
  CHECK(td.num_values() == (double)data.size(), "tdigest count");
  CHECK(td.total_arr_size() < 200, "tdigest compresses to few centroids");

  // merge of partitions stays accurate vs exact
  TDigest a(100.0), b(100.0);
  for (size_t i = 0; i < data.size(); i++) (i % 2 ? a : b).add(data[i]);
  a.merge(b);
  CHECK(std::fabs(a.get_quantile_value(0.5) - eq.get_quantile_value(0.5)) < 0.02,
        "merged tdigest accurate at median");
  CHECK(std::fabs(a.get_quantile_value(0.99) - eq.get_quantile_value(0.99)) < 0.02,
        "merged tdigest accurate at p99");
}

// KLL gives a rank guarantee (~1/k), not a value guarantee, so check that the
// estimated quantile lands at roughly the right rank in the data.
static double actual_rank(const std::vector<double>& sorted, double value) {
  size_t c = 0;
  for (double v : sorted) if (v <= value) c++;
  return static_cast<double>(c) / sorted.size();
}

static void test_kll_rank_accuracy_and_merge() {
  std::printf("test_kll_rank_accuracy_and_merge\n");
  auto data = uniform_data(300000);
  std::vector<double> sorted = data;
  std::sort(sorted.begin(), sorted.end());

  KLL kll(256);
  for (double v : data) kll.add(v);
  for (double q : {0.1, 0.5, 0.9, 0.99}) {
    double est = kll.get_quantile_value(q);
    CHECK(std::fabs(actual_rank(sorted, est) - q) < 0.02, "kll rank within 2% of target");
  }
  CHECK(kll.num_values() == (double)data.size(), "kll count");
  CHECK(kll.total_arr_size() < data.size() / 100, "kll keeps far fewer than all values");

  KLL a(256), b(256);
  for (size_t i = 0; i < data.size(); i++) (i % 2 ? a : b).add(data[i]);
  a.merge(b);
  CHECK(a.num_values() == (double)data.size(), "merged kll count");
  CHECK(std::fabs(actual_rank(sorted, a.get_quantile_value(0.5)) - 0.5) < 0.03,
        "merged kll rank accurate at median");
}

int main() {
  test_exact_quantiles_basic();
  test_ddsketch_accuracy_vs_exact();
  test_ddsketch_merge_equals_single();
  test_ddsketch_negative_values();
  test_histogram_basic_and_merge();
  test_histogram_out_of_range();
  test_tdigest_accuracy_and_merge();
  test_kll_rank_accuracy_and_merge();

  if (g_failures == 0) {
    std::printf("\nALL TESTS PASSED\n");
  } else {
    std::printf("\n%d CHECK(S) FAILED\n", g_failures);
  }
  return g_failures;
}

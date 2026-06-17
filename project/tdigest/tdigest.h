#ifndef DATA_SUMMARIES_TDIGEST_H
#define DATA_SUMMARIES_TDIGEST_H

#include <cstddef>
#include <vector>

// A merging t-digest (Dunning & Ertl, 2019: https://arxiv.org/abs/1902.04023).
//
// Quantiles are summarised by a small set of weighted centroids whose size is
// governed by the k1 scale function: centroids near the tails are kept small
// (high accuracy where it matters), centroids near the median may absorb more
// weight. Memory is bounded by the compression parameter, independent of the
// data size, and two digests merge by concatenating centroids and recompressing
// - so it parallelises like the other sketches here.
class TDigest {
public:
  explicit TDigest(double compression = 100.0);

  void add(double val, double weight = 1.0);
  double get_quantile_value(double quantile);
  void merge(const TDigest& other);

  size_t total_arr_size();      // number of centroids (after compression)
  double num_values();

private:
  void process();               // fold the unmerged buffer into the centroids
  static double k1(double q, double normalizer);

  double compression_;
  double normalizer_;           // compression / (2*pi) for the k1 scale
  std::vector<double> cmean_;   // centroid means, kept sorted
  std::vector<double> cweight_; // centroid weights
  std::vector<double> buf_mean_;
  std::vector<double> buf_weight_;
  double total_weight_ = 0.0;
  double min_ = 0.0;
  double max_ = 0.0;
  bool empty_ = true;
};

#endif  // DATA_SUMMARIES_TDIGEST_H

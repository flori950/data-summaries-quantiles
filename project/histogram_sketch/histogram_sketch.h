#ifndef DATA_SUMMARIES_HISTOGRAM_SKETCH_H
#define DATA_SUMMARIES_HISTOGRAM_SKETCH_H

#include <cstddef>
#include <vector>

// A fixed-width histogram quantile sketch: a third, deliberately simple point of
// comparison for DDsketch (relative-error buckets) and ExactQuantiles (store
// everything). Memory is bounded by the bin count and independent of the data
// size; merge() is an element-wise add, so it parallelizes like DDsketch.
//
// Values are bucketed into `nbins` equal-width bins spanning [lo, hi]; values
// outside the range are clamped into the edge bins (and counted as out-of-range
// so callers can detect a badly chosen range). The absolute quantile error is
// therefore bounded by the bin width (hi - lo) / nbins.
class HistogramSketch {
public:
  HistogramSketch(double lo = -50.0, double hi = 50.0, size_t nbins = 100000);

  void add(double val, double weight = 1.0);
  double get_quantile_value(double quantile);
  void merge(const HistogramSketch& other);
  bool mergeable(const HistogramSketch& other) const;

  size_t total_arr_size() const { return bins_.size(); }
  double num_values() const { return count_; }
  size_t out_of_range() const { return out_of_range_; }

private:
  double lo_;
  double hi_;
  double width_;
  std::vector<double> bins_;
  double count_ = 0.0;
  size_t out_of_range_ = 0;
};

#endif  // DATA_SUMMARIES_HISTOGRAM_SKETCH_H

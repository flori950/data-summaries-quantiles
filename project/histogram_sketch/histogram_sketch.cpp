#include "histogram_sketch.h"

#include <cmath>
#include <limits>
#include <stdexcept>

HistogramSketch::HistogramSketch(double lo, double hi, size_t nbins)
    : lo_(lo), hi_(hi), bins_(nbins, 0.0) {
  if (nbins == 0 || !(hi > lo)) {
    throw std::invalid_argument("HistogramSketch needs nbins > 0 and hi > lo");
  }
  width_ = (hi_ - lo_) / static_cast<double>(nbins);
}

void HistogramSketch::add(double val, double weight) {
  long idx;
  if (val <= lo_) {
    idx = 0;
    if (val < lo_) out_of_range_++;
  } else if (val >= hi_) {
    idx = static_cast<long>(bins_.size()) - 1;
    if (val > hi_) out_of_range_++;
  } else {
    idx = static_cast<long>((val - lo_) / width_);
    if (idx < 0) idx = 0;
    if (idx >= static_cast<long>(bins_.size())) idx = static_cast<long>(bins_.size()) - 1;
  }
  bins_[idx] += weight;
  count_ += weight;
}

double HistogramSketch::get_quantile_value(double quantile) {
  if (quantile < 0.0 || quantile > 1.0) {
    throw std::invalid_argument("quantile must be in [0, 1]");
  }
  if (count_ == 0.0) return std::numeric_limits<double>::quiet_NaN();

  // Same rank convention as DDsketch / ExactQuantiles: rank = q * (N - 1).
  double rank = quantile * (count_ - 1.0);
  double running = 0.0;
  for (size_t i = 0; i < bins_.size(); i++) {
    running += bins_[i];
    if (running > rank) {
      // return the bin centre as the representative value
      return lo_ + (static_cast<double>(i) + 0.5) * width_;
    }
  }
  return hi_ - 0.5 * width_;  // fall through: last populated bin
}

bool HistogramSketch::mergeable(const HistogramSketch& other) const {
  return lo_ == other.lo_ && hi_ == other.hi_ && bins_.size() == other.bins_.size();
}

void HistogramSketch::merge(const HistogramSketch& other) {
  if (!mergeable(other)) {
    throw std::invalid_argument("cannot merge HistogramSketches with different ranges/bins");
  }
  for (size_t i = 0; i < bins_.size(); i++) bins_[i] += other.bins_[i];
  count_ += other.count_;
  out_of_range_ += other.out_of_range_;
}

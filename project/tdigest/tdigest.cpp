#include "tdigest.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace {
constexpr double kPi = 3.14159265358979323846;
}

TDigest::TDigest(double compression) : compression_(compression) {
  normalizer_ = compression_ / (2.0 * kPi);
}

// k1 scale function: maps a quantile to "k-space"; a centroid is allowed to grow
// while the span of k it covers stays <= 1.
double TDigest::k1(double q, double normalizer) {
  if (q <= 0.0) return -normalizer * (kPi / 2.0);
  if (q >= 1.0) return normalizer * (kPi / 2.0);
  return normalizer * std::asin(2.0 * q - 1.0);
}

void TDigest::add(double val, double weight) {
  if (weight <= 0.0) return;
  buf_mean_.push_back(val);
  buf_weight_.push_back(weight);
  if (empty_) {
    min_ = max_ = val;
    empty_ = false;
  } else {
    min_ = std::min(min_, val);
    max_ = std::max(max_, val);
  }
  // recompress once the buffer is large relative to the centroid budget
  if (buf_mean_.size() > static_cast<size_t>(10.0 * compression_) + 16) process();
}

void TDigest::process() {
  if (buf_mean_.empty()) return;

  // gather existing centroids + buffered points, sorted by mean
  const size_t n = cmean_.size() + buf_mean_.size();
  std::vector<size_t> order(n);
  std::vector<double> all_mean;
  std::vector<double> all_weight;
  all_mean.reserve(n);
  all_weight.reserve(n);
  all_mean.insert(all_mean.end(), cmean_.begin(), cmean_.end());
  all_mean.insert(all_mean.end(), buf_mean_.begin(), buf_mean_.end());
  all_weight.insert(all_weight.end(), cweight_.begin(), cweight_.end());
  all_weight.insert(all_weight.end(), buf_weight_.begin(), buf_weight_.end());
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(),
            [&](size_t a, size_t b) { return all_mean[a] < all_mean[b]; });

  double total = std::accumulate(all_weight.begin(), all_weight.end(), 0.0);

  std::vector<double> out_mean;
  std::vector<double> out_weight;
  out_mean.reserve(order.size());
  out_weight.reserve(order.size());

  out_mean.push_back(all_mean[order[0]]);
  out_weight.push_back(all_weight[order[0]]);
  double weight_before = 0.0;  // total weight of completed centroids

  for (size_t i = 1; i < order.size(); i++) {
    size_t idx = order[i];
    double proposed = out_weight.back() + all_weight[idx];
    double q0 = weight_before / total;
    double q2 = (weight_before + proposed) / total;
    if (k1(q2, normalizer_) - k1(q0, normalizer_) <= 1.0) {
      // merge into the current centroid (weighted mean update)
      out_mean.back() += (all_mean[idx] - out_mean.back()) * all_weight[idx] / proposed;
      out_weight.back() = proposed;
    } else {
      weight_before += out_weight.back();
      out_mean.push_back(all_mean[idx]);
      out_weight.push_back(all_weight[idx]);
    }
  }

  cmean_.swap(out_mean);
  cweight_.swap(out_weight);
  total_weight_ = total;
  buf_mean_.clear();
  buf_weight_.clear();
}

double TDigest::get_quantile_value(double quantile) {
  if (quantile < 0.0 || quantile > 1.0) return std::numeric_limits<double>::quiet_NaN();
  process();
  if (cmean_.empty()) return std::numeric_limits<double>::quiet_NaN();
  if (cmean_.size() == 1) return cmean_[0];
  if (quantile <= 0.0) return min_;
  if (quantile >= 1.0) return max_;

  double index = quantile * total_weight_;

  // left tail: between min_ and the first centroid
  double weight_so_far = cweight_[0] / 2.0;
  if (index < weight_so_far) {
    return min_ + (index / weight_so_far) * (cmean_[0] - min_);
  }

  for (size_t i = 0; i < cmean_.size() - 1; i++) {
    double dw = (cweight_[i] + cweight_[i + 1]) / 2.0;
    if (index < weight_so_far + dw) {
      double z1 = index - weight_so_far;
      double z2 = weight_so_far + dw - index;
      return (cmean_[i] * z2 + cmean_[i + 1] * z1) / dw;
    }
    weight_so_far += dw;
  }

  // right tail: between the last centroid and max_
  double last_w = cweight_.back() / 2.0;
  double z1 = index - weight_so_far;
  return cmean_.back() + (z1 / last_w) * (max_ - cmean_.back());
}

void TDigest::merge(const TDigest& other) {
  if (other.empty_ && other.buf_mean_.empty()) return;
  // fold the other digest's centroids and any buffered points into our buffer
  for (size_t i = 0; i < other.cmean_.size(); i++) add(other.cmean_[i], other.cweight_[i]);
  for (size_t i = 0; i < other.buf_mean_.size(); i++) add(other.buf_mean_[i], other.buf_weight_[i]);
  if (!other.empty_) {
    min_ = empty_ ? other.min_ : std::min(min_, other.min_);
    max_ = empty_ ? other.max_ : std::max(max_, other.max_);
    empty_ = false;
  }
  process();
}

size_t TDigest::total_arr_size() {
  process();
  return cmean_.size();
}

double TDigest::num_values() {
  process();
  return total_weight_;
}

#include "kll.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

KLL::KLL(int k, uint32_t seed) : k_(k < 2 ? 2 : k), rng_(seed) {
  grow();
}

int KLL::capacity(size_t h) const {
  // top level (largest) has capacity k; capacities shrink toward the bottom
  size_t depth = compactors_.size() - 1 - h;
  int cap = static_cast<int>(std::ceil(k_ * std::pow(c_, static_cast<double>(depth))));
  return std::max(cap, 2);
}

void KLL::grow() {
  compactors_.emplace_back();
  max_size_ = 0;
  for (size_t h = 0; h < compactors_.size(); h++) max_size_ += capacity(h);
}

size_t KLL::stored() const {
  size_t s = 0;
  for (const auto& c : compactors_) s += c.size();
  return s;
}

void KLL::add(double val) {
  compactors_[0].push_back(val);
  n_++;
  if (stored() >= max_size_) compress();
}

void KLL::compress() {
  for (size_t h = 0; h < compactors_.size(); h++) {
    if (compactors_[h].size() >= static_cast<size_t>(capacity(h))) {
      if (h + 1 >= compactors_.size()) grow();
      auto& level = compactors_[h];
      std::sort(level.begin(), level.end());
      // promote every other element, starting at a random offset (coin flip)
      size_t offset = rng_() & 1u;
      auto& up = compactors_[h + 1];
      for (size_t i = offset; i < level.size(); i += 2) up.push_back(level[i]);
      level.clear();
      return;  // one compaction is enough per overflow
    }
  }
}

void KLL::compress_full() {
  while (stored() >= max_size_) compress();
}

double KLL::get_quantile_value(double quantile) {
  if (quantile < 0.0 || quantile > 1.0) return std::numeric_limits<double>::quiet_NaN();
  if (n_ == 0) return std::numeric_limits<double>::quiet_NaN();

  // gather (value, weight) pairs; an item at level h has weight 2^h
  std::vector<std::pair<double, long long>> items;
  items.reserve(stored());
  for (size_t h = 0; h < compactors_.size(); h++) {
    long long w = 1LL << h;
    for (double v : compactors_[h]) items.emplace_back(v, w);
  }
  std::sort(items.begin(), items.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  // same rank convention as the other structures: rank = q * (N - 1)
  double rank = quantile * (static_cast<double>(n_) - 1.0);
  long long running = 0;
  for (const auto& it : items) {
    running += it.second;
    if (static_cast<double>(running) > rank) return it.first;
  }
  return items.back().first;
}

void KLL::merge(const KLL& other) {
  // make room for the other sketch's levels
  while (compactors_.size() < other.compactors_.size()) grow();
  for (size_t h = 0; h < other.compactors_.size(); h++) {
    compactors_[h].insert(compactors_[h].end(),
                          other.compactors_[h].begin(), other.compactors_[h].end());
  }
  n_ += other.n_;
  compress_full();
}

size_t KLL::total_arr_size() { return stored(); }

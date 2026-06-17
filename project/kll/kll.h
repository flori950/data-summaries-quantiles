#ifndef DATA_SUMMARIES_KLL_H
#define DATA_SUMMARIES_KLL_H

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

// A KLL sketch (Karnin, Lang & Liberty, 2016: https://arxiv.org/abs/1603.05346).
//
// A hierarchy of "compactors" (sorted buffers). Items enter the bottom level;
// when a level overflows it is sorted and compacted - every other element is
// promoted one level up, where each item carries twice the weight. Capacities
// shrink geometrically toward the bottom, giving O(k) memory for a relative-rank
// error of ~1/k. Randomised (the promoted half is chosen by a coin flip), so two
// sketches merge by folding levels together and recompacting.
class KLL {
public:
  explicit KLL(int k = 256, uint32_t seed = 0xC0FFEE);

  void add(double val);
  double get_quantile_value(double quantile);
  void merge(const KLL& other);

  size_t total_arr_size();  // number of items currently retained
  double num_values() const { return static_cast<double>(n_); }

private:
  void grow();
  int capacity(size_t h) const;     // capacity of the compactor at level h
  void compress();                  // one compaction pass
  void compress_full();             // compact until under budget (used by merge)
  size_t stored() const;

  int k_;
  double c_ = 2.0 / 3.0;
  std::vector<std::vector<double>> compactors_;
  size_t max_size_ = 0;
  long long n_ = 0;                 // total weight (number of values added)
  std::mt19937 rng_;
};

#endif  // DATA_SUMMARIES_KLL_H

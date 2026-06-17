
#include <vector>
#include <limits>
#ifndef DATA_SUMMARIES_STORE_H
#define DATA_SUMMARIES_STORE_H

class Dense_store {

public:
  Dense_store(long chunk_size = 128);
  ~Dense_store();

  void copy(Dense_store &store);
  long length();
  void add(long key, double weight = 1.0);
  long get_index(long key);
  long get_new_length(long new_min_key, long new_max_key);
  void extend_range(long key, long second_key = std::numeric_limits<long>::max());
  void adjust(long new_min_key, long new_max_key);
  void shift_bins(double shift);
  void center_bins(long new_min_key, long new_max_key); // TODO rethink what these functions actually do, maybe we can go without copying
  long key_at_rank(double rank, bool lower = true);
  void merge(Dense_store &store);

  double count = 0;
  double max_key_ = (double) (std::numeric_limits<long>::min() + 800); //py uses 'inf' here, this cast prevents overflow
  double min_key_ = (double) (std::numeric_limits<long>::max() - 800); // contrary to common sense, it only works with a slight offset, eg. 500 is not enough

  long chunk_size_;
  double offset_ = 0;
  std::vector<double> bin_array_;

protected: // base class and children can access these
private: //only this class itself can access these below
};
#endif //DATA_SUMMARIES_STORE_H

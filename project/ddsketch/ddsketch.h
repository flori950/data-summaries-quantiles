#include <limits>
#include "logarithmic_mapping.h"
#include "dense_store.h"
#ifndef DATA_SUMMARIES_DDSKETCH_H
#define DATA_SUMMARIES_DDSKETCH_H

class DDsketch {
public:
  DDsketch(double relative_accuracy = 0.01);
  ~DDsketch();

  double num_values();
  double avg();
  double sum();
  void add(double val, double weight = 1.0);
  double get_quantile_value(double quantile);
  void merge (DDsketch &other);
  bool mergeable(DDsketch& other);
  void copy(DDsketch& other);
  size_t total_arr_size();

protected: // base class and children can access these
private: //only this class itself can access these below

  Logarithmic_mapping mapping_;
  Dense_store store_;
  Dense_store negative_store_;
  double zero_count_;
  double count_;
  double max_ = (double) (std::numeric_limits<long>::min() + 800); // 800 determined empirically, avoids over/underflows
  double min_ = (double) (std::numeric_limits<long>::max() - 800); // necessary as py uses 'inf'
  double sum_= 0.0;
  double relative_accuracy_;

};

#endif //DATA_SUMMARIES_DDSKETCH_H

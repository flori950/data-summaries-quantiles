
#include "ddsketch.h"
#include "logarithmic_mapping.h"
#include "dense_store.h"
#include <iostream>

DDsketch::DDsketch(double relative_accuracy) { // default: = 0.01
  mapping_ = Logarithmic_mapping(relative_accuracy);
  store_ = Dense_store();
  negative_store_ = Dense_store();
  zero_count_ = 0;// do not know what that does, "count of zero values" -> opti?

  relative_accuracy_ = mapping_.relative_accuracy_;// TODO maybe just a ptr to it?
  count_ = negative_store_.count + zero_count_ + store_.count; // TODO getters?
  //min/max/sum already set in .h
};

DDsketch::~DDsketch(){
  // empty deconstructor for now
//  ~mapping_; // TODO check if that is actually needed
//  ~store_;
//  ~negative_store_;
}

double DDsketch::num_values() { return count_;}

double DDsketch::avg() { return (sum_ / count_);}
double DDsketch::sum() { return sum_;}

void DDsketch::add(double val, double weight){
  if (weight <= 0.0) {
    std::cout << "FAIL: ddsketch.add() called with invalid weight 0=>" << weight << std::endl;
    exit(1);
  }
  //add
  if       (val > mapping_.min_possible_)          store_.add(mapping_.key(val), weight);
  else if (val < -mapping_.min_possible_) negative_store_.add(mapping_.key(-val), weight);
  else zero_count_ += weight;

  //update stats
  count_ +=weight;
  sum_   += val * weight;

  if (val < min_) min_ = val;
  if (val > max_) max_ = val;

}


double DDsketch::get_quantile_value(double quantile){
  if (quantile < 0.0 || quantile > 1.0) {
    std::cout << "FAIL: ddsketch.get_quantile_value() called with invalid quantile < 0 or 1 <, quantile:" << quantile << std::endl;
    exit(1);
  }
  if (!count_) return std::numeric_limits<double>::min(); // todo think: placeholder for none atm


  double rank = quantile * (count_ - 1);
  double reversed_rank, quantile_value;
  long key;
  if (rank < negative_store_.count){
    reversed_rank = negative_store_.count - rank - 1;
    key = negative_store_.key_at_rank(reversed_rank, false);
    quantile_value = -mapping_.value(key);

  } else if (rank < (zero_count_ + negative_store_.count)){
    return 0;

  } else {
    key = store_.key_at_rank((rank - zero_count_ - negative_store_.count));
    quantile_value = mapping_.value(key);
    return quantile_value;
  }

  return quantile_value;
}

void DDsketch::merge (DDsketch& other){

  if (!this->mergeable(other)) {
    std::cout << "FAIL: ddsketch.merge() called with faulty other sketch. \n Cannot merge two DDSketches with different parameters" << std::endl;
    exit(1);
  }

  if (other.count_ == 0) return;

  if (this->count_ == 0) {
    this->copy(other);
    return;
  }

  // merge the stores
  this->store_.merge(other.store_);
  this->negative_store_.merge(other.negative_store_);
  this->zero_count_ += other.zero_count_;

  //merge summary stats
  this->count_ += other.count_;
  this->sum_ += other.sum_;
  if (other.min_ < this->min_) this->min_ = other.min_;
  if (other.max_ > this->max_) this->max_ = other.max_;
}

//Two sketches can be merged only if their gammas are equal.
bool DDsketch::mergeable(DDsketch& other) {
  return this->mapping_.gamma_ == other.mapping_.gamma_;
}

//copy the input sketch into this one
void DDsketch::copy(DDsketch& other) {
  this->store_.copy(other.store_);
  this->negative_store_.copy(other.negative_store_);
  this->zero_count_ = other.zero_count_;
  this->min_ = other.min_;
  this->max_ = other.max_;
  this->count_ = other.count_;
  this->sum_ = other.sum_;
}

size_t DDsketch::total_arr_size() { return store_.bin_array_.size() + negative_store_.bin_array_.size(); }

#include <iostream>
#include <algorithm>
#include <cmath>
#include"dense_store.h"
Dense_store::Dense_store(long chunk_size) {
  chunk_size_ = chunk_size;
  // other stuff is apparently static and done by .h
}
Dense_store::~Dense_store() {}

void  Dense_store::copy(Dense_store &store) {

  this->bin_array_ =  store.bin_array_;
  this->count = store.count;
  this->min_key_ = store.min_key_;
  this->max_key_ = store.max_key_;
  this->offset_ = store.offset_;
}

long Dense_store::length(){
  return bin_array_.size();
}

void Dense_store::add(long key, double weight){

  long idx = get_index(key);
  bin_array_[idx] += weight;
  count += weight;
}

long Dense_store::get_index(long key) { //correct

  if ((double)key < min_key_){ // test if overflows appear or not
    extend_range(key);
  } else if((double)key > max_key_){
    extend_range(key);
  }
  return (key - (long)offset_);
}

long Dense_store::get_new_length(long new_min_key, long new_max_key){

  long desired_length = new_max_key - new_min_key + 1; // TODO investigate for potential overflows here
  return (chunk_size_ * std::ceil(((double) desired_length) / (double) chunk_size_));
}

void Dense_store::extend_range(long key, long second_key){

  long casted_max =(long) max_key_;
  long casted_min =(long) min_key_;
  if (second_key == std::numeric_limits<long>::max()) second_key = key;
  long new_min_key = std::min({key, second_key, casted_min}),
  new_max_key = std::max({key, second_key, casted_max});

  if (!bin_array_.size()) {
    // init bins
    int a = get_new_length(new_min_key, new_max_key);
    bin_array_.resize(a); //should be  128 after this
    offset_ = new_min_key;
    adjust(new_min_key, new_max_key);
  } else if (new_min_key >= min_key_ and new_max_key < offset_ + bin_array_.size()) {
    // no need to change the range; just update min/max keys
    min_key_ = new_min_key;
    max_key_ = new_max_key;
  } else {
    // grow the bins
    long new_length = get_new_length(new_min_key, new_max_key);
    if (new_length > (long)bin_array_.size()) {
      bin_array_.resize(new_length, 0.0);
    }
    adjust(new_min_key, new_max_key);
  }
}

void Dense_store::adjust(long new_min_key, long new_max_key){
  /*Adjust the bins, the offset, the min_key, and max_key, without resizing the
  bins, in order to try making it fit the specified range.
  */
  center_bins(new_min_key, new_max_key);
  min_key_ = new_min_key;
  max_key_ = new_max_key;
}

void Dense_store::shift_bins(double shift){

  //shift the bins; this changes the offset
  if (shift > 0){
    std::vector<double> new_vec(shift,0);
    new_vec.reserve(new_vec.size() + (bin_array_.size() - shift)); // should be the same as just bin_array; look up when perf matters
    for (long i = 0; i < (bin_array_.size() - shift); i++){//TODO ask if insert can be faster, eg using: https://stackoverflow.com/questions/2551775/appending-a-vector-to-a-vector
      new_vec.push_back(bin_array_[i]);// todo probably double-check limits if inaccuracies arise
    }
    bin_array_ = new_vec;
  } else {
    std::vector<double> new_vec;
    new_vec.reserve(bin_array_.size());
    long absolute_shift =(long)std::abs(shift);
    for(long i = absolute_shift; i <  (long)bin_array_.size(); i++){
      new_vec.push_back(bin_array_[i]); //TODO performance theres gotta be a faster way to copy individual slices of a vector
    }
    new_vec.resize(bin_array_.size(), 0.0); // TODO instead of copying vec, use deque to pop off shift-amount of elems from the front. TODO performance here
    bin_array_ = new_vec;
  }
  offset_ -= shift;
}

void Dense_store::center_bins(long new_min_key, long new_max_key){ // TODO rethink what these functions actually do, maybe we can go without copying

  //center the bins; this changes the offset
  long middle_key = new_min_key + std::floor(((double)new_max_key - (double) new_min_key + 1.0) / 2.0 );
  shift_bins(offset_ + std::floor( (double) bin_array_.size() / 2) - middle_key);
  }

long Dense_store::key_at_rank(double rank, bool lower ){

  double running_ct = 0;
  for (long i = 0; i< (long)bin_array_.size(); i++) {
    running_ct += bin_array_[i];

    if ((lower && running_ct > rank) || (!lower && running_ct >= (rank + 1))) {
      return (i + offset_);
    }
  }

  return max_key_;
}

/* immediate optimization ideas
 *  copy the smaller into the bigger, not the other way round
 *  protect the store which will be copied into with locks for parallelity
 */
void Dense_store::merge(Dense_store &store){

  if (store.count == 0) return;
  if (this->count == 0) {
    this->copy(store);
    return;
  }

  if (store.min_key_ < this->min_key_ || store.max_key_ > this->max_key_) {
    this->extend_range(store.min_key_, store.max_key_);
  }

  for (long key = store.min_key_; key < store.max_key_ + 1; key++) {
   this->bin_array_[key - this->offset_] += store.bin_array_[key - store.offset_];
  }

  this->count += store.count;
}

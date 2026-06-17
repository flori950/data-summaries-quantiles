
#include <iostream>
#include "logarithmic_mapping.h"
#include <cmath>


Logarithmic_mapping::Logarithmic_mapping() {} // empty, called  for initial setup

Logarithmic_mapping::Logarithmic_mapping( double relative_accuracy, double offset) {
  if(relative_accuracy <= 0 || relative_accuracy >= 1){
    std::cout << "FAIL: relative accuracy must be stricly greater than 0 and strictly smaller than 1, in this case:"
                 << "\n relative_accuracy=" << relative_accuracy << ", aborting" << std::endl;
    exit(0);
  }
  relative_accuracy_ = relative_accuracy;
  offset_ = offset;
  double gamma_mantissa_ = 2 * relative_accuracy / (1 - relative_accuracy);
  gamma_ = 1 + gamma_mantissa_;
  multiplier_ = (1 / std::log1p(gamma_mantissa_));
  min_possible_ = std::numeric_limits<double>::min() * gamma_;
  max_possible_ = std::numeric_limits<double>::max() / gamma_;
  multiplier_ = multiplier_ * std::log(2); // in logarithmic-mapping, rest is from parent-class keymapping
}

Logarithmic_mapping::~Logarithmic_mapping() {
//deconstructor, needed?
}

long Logarithmic_mapping::key(double value){
  return ( ceil(log_gamma(value)) + offset_);
}

double Logarithmic_mapping::value (long key){
  // Reference DDSketch KeyMapping.value: pow_gamma(key - offset) * 2/(1+gamma).
  // (Previously the 2/(1+gamma) factor was misplaced inside the offset term,
  //  biasing every returned quantile value high by ~(1 - 2/(1+gamma)).)
  return pow_gamma(key - offset_) * (2.0 / (1.0 + gamma_));
}

double Logarithmic_mapping::log_gamma(double value) {
  return (std::log2(value) * multiplier_);
}

double Logarithmic_mapping::pow_gamma(double value) {
  return (std::pow(2,value / multiplier_));
}

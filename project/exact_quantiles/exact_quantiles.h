
#include <vector>


class ExactQuantiles {
public:
  ExactQuantiles();
  ~ExactQuantiles(){}

  void add(double val);
  void sort(); // sorts array multithreaded
  int get_N() const;
  double get_quantile_value(double quantile);
  size_t total_arr_size();

protected: // base class and children can access these
private: //only this class itself can access these below
  std::vector<double> val_vec_;
  bool sorted_ = false;
};

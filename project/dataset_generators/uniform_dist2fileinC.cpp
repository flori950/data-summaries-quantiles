// Generates a uniformly distributed dataset and writes it space-separated,
// wrapped in [ ], to a file named after its parameters - matching the format
// produced by uniform_dist2file.py and consumed by main.cpp::readfile().
//
// Build (standalone, no external dependencies):
//   c++ -std=c++17 -O2 uniform_dist2fileinC.cpp -o gen && ./gen

#include <fstream>
#include <iostream>
#include <random>
#include <string>

int main() {
  // adjust as needed
  const double min_val = -1.0;
  const double max_val = 1.0;
  const std::size_t sample_size = 1000;

  std::default_random_engine eng;  // unseeded for reproducibility across runs
  std::uniform_real_distribution<double> dist(min_val, max_val);

  const std::string filename =
      "uniform_dist_[" + std::to_string(static_cast<long>(min_val)) + "," +
      std::to_string(static_cast<long>(max_val)) + "[_" +
      std::to_string(sample_size) + "_samples.txt";

  std::ofstream out(filename);
  if (!out.is_open()) {
    std::cerr << "Unable to open file '" << filename << "'\n";
    return 1;
  }

  out << "[";
  for (std::size_t i = 0; i < sample_size; ++i) {
    if (i != 0) out << " ";
    out << dist(eng);
  }
  out << "]";
  out.close();

  std::cout << "written " << sample_size << " samples to '" << filename << "'\n";
  return 0;
}

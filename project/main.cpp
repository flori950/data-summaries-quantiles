
#include <iostream>
#include <map>
#include <set>
#include <numeric>
#include <random>
#include <algorithm>
#include <sstream>
#include <math.h>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include "pthread.h"
#include <unistd.h>
#include "ddsketch/ddsketch.h"
#include "exact_quantiles/exact_quantiles.h"
#include "histogram_sketch/histogram_sketch.h"
#include "tdigest/tdigest.h"
#include "kll/kll.h"
#include <chrono>
#include <algorithm>
#include <random>

using namespace std;


void baseline1() {
  DDsketch sk1 = DDsketch();

  std::vector<double> values { 0.82237959, -0.61125007, 0.4383297, 0.63498452, 1.30493411};
  for (auto& e : values) {
    std::cout << " adding elem: " << e << "\n";
    sk1.add(e);
    std::cout << " adding done of elem: " << e << "\n";
  }

  std::vector<double> qvs {0.5, 0.75, 0.9, 0.95, 0.99, 1.0};
  std::string qv_str = "[";
  for (auto& e : qvs) {
    double qv = sk1.get_quantile_value(e);
    qv_str.append(std::to_string(qv));
    qv_str.append(", ");
  }
  qv_str.pop_back();
  qv_str.pop_back();
  qv_str.append("]");

  std::cout << "quantiles here: qv=" << qv_str << std::endl;
}

// fills a vector of a given size with reproducable uniformly distributed random numbers
void generate_uniform_dataset(std::vector<double> &values ) {
  std::chrono::steady_clock::time_point begin_gen = std::chrono::steady_clock::now();

  std::default_random_engine eng;
  int start = -1;
  int end = 1;
  std::uniform_real_distribution<double> urd(start, end);

  // populate vector without random seed for reproducability
  for (size_t e = 0; e < values.size(); e++) values[e] = urd(eng);
  std::chrono::steady_clock::time_point end_gen = std::chrono::steady_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_gen - begin_gen).count();
  auto s  = std::chrono::duration_cast<std::chrono::seconds>(end_gen - begin_gen).count();
  cout << "Dataset generation finished, size=" << values.size() << " range  [" << start << ", " << end <<  "]"
  << "\nTime taken:" << ms << "[ms] " << s << "[s]" << endl;
}

// Supported input distributions. Approximate sketches like DDsketch are most
// interesting on skewed data, so normal, exponential and lognormal (a common
// model for service latencies) are offered too.
enum class Distribution { Uniform, Normal, Exponential, Lognormal };

const char* distribution_name(Distribution d) {
  switch (d) {
    case Distribution::Uniform:     return "uniform";
    case Distribution::Normal:      return "normal";
    case Distribution::Exponential: return "exponential";
    case Distribution::Lognormal:   return "lognormal";
  }
  return "uniform";
}

// fills a vector with reproducible samples from the requested distribution
void generate_dataset(std::vector<double>& values, Distribution dist) {
  if (dist == Distribution::Uniform) {  // keep the original uniform path/logging
    generate_uniform_dataset(values);
    return;
  }
  std::default_random_engine eng;  // unseeded for reproducibility across runs
  std::normal_distribution<double> nd(0.0, 1.0);
  std::exponential_distribution<double> ed(1.0);
  std::lognormal_distribution<double> lnd(0.0, 1.0);
  for (size_t e = 0; e < values.size(); e++) {
    switch (dist) {
      case Distribution::Normal:      values[e] = nd(eng);  break;
      case Distribution::Exponential: values[e] = ed(eng);  break;
      case Distribution::Lognormal:   values[e] = lnd(eng); break;
      default:                        values[e] = nd(eng);  break;
    }
  }
  cout << "Dataset generation finished (" << distribution_name(dist)
       << "), size=" << values.size() << endl;
}

// Loads a real dataset from a file of whitespace/comma separated numbers,
// tolerating the optional surrounding [ ] of the generator format.
std::vector<double> load_dataset_file(const std::string& path) {
  std::vector<double> values;
  std::ifstream in(path);
  if (!in) {
    std::cout << "FAIL: could not open dataset file '" << path << "'" << std::endl;
    exit(1);
  }
  std::string tok;
  while (in >> tok) {
    // strip brackets / trailing commas
    std::string clean;
    for (char c : tok) {
      if (c != '[' && c != ']' && c != ',') clean.push_back(c);
    }
    if (clean.empty()) continue;
    try {
      values.push_back(std::stod(clean));
    } catch (const std::exception&) {
      // skip non-numeric tokens (headers etc.)
    }
  }
  std::cout << "loaded " << values.size() << " values from '" << path << "'" << std::endl;
  return values;
}

inline bool check_possible_double(string& s) {
  vector<char> allowed_chars{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '.', '-', ' '};
  for (size_t i = 0; i < s.size(); i++) {
    bool ischar = false;
    for (auto &elem: allowed_chars) {
      if (s[i] == elem) {
        ischar = true;
        break;
      }
    }
    if (!ischar) {
      cout << "rejected string: " << s << endl;
      return false;
    }

  }
  return true;
}
// fills a vector of a given size with reproducable uniformly distributed random INTEGERS
void generate_uniform_dataset_integer(std::vector<int> &values) {
  std::chrono::steady_clock::time_point begin_gen = std::chrono::steady_clock::now();

  std::default_random_engine eng;
  int start = -10000000; //as we can only work with whole numbers, we make the range arbitrarily bigger
  int end = 10000000;
  std::uniform_real_distribution<double> urd(start, end);

  // populate vector without random seed for reproducability
  for (size_t e = 0; e < values.size(); e++){
    values[e] = (int) urd(eng);
  }
  std::chrono::steady_clock::time_point end_gen = std::chrono::steady_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_gen - begin_gen).count();
  auto s  = std::chrono::duration_cast<std::chrono::seconds>(end_gen - begin_gen).count();
  cout << "INTEGER Dataset generation finished, size:" << values.size() << " range  [" << start << ", " << end <<  "]"
       << "\nTime taken:" << ms << "[ms] " << s << "[s]" << endl;
}

// read dataset from file created by python. Slow ingest, use for validation between python/c++ only
void readfile(std::vector<double> &values, string file) {
  ifstream infile(file);
  string token;
  vector<string> token_vec;
  while ( infile >> token) {
    token_vec.push_back(token);
  }

//  token_vec[0] = token_vec[0].substr(1,token_vec[0].size());
//  token_vec[token_vec.size()-1] = token_vec[token_vec.size()-1].substr(0,token_vec[0].size() - 1);

  for (size_t e = 0; e < token_vec.size(); e++) {
    string elem = token_vec[e];
    for (size_t j = 0; j < elem.size(); j++) {
      if (token_vec[e][j] == '\n') {
        cout << "CUT IT" << endl;
        token_vec[e] = token_vec[e].substr(0, token_vec[e].size() - 1);
      }
    }
//    cout << " adding " << token_vec[e] << endl;
    if (check_possible_double(token_vec[e])){
      double newdouble =  stod(token_vec[e]);
      values.push_back(newdouble);
    }
  }
  cout << "done reading file; size of resulting vector=" << values.size() << endl;
}

void baseline2(const string& file) {
  DDsketch sk1 = DDsketch();
  std::vector<double> values;
  readfile(values, file);

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  for (auto& e : values) sk1.add(e);
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

  std::vector<double> qvs {0.5, 0.75, 0.9, 0.95, 0.99, 1.0};
  std::string qv_str = "[";
  std::chrono::steady_clock::time_point qvbegin = std::chrono::steady_clock::now();
  for (auto& e : qvs) {
    double qv = sk1.get_quantile_value(e);
    qv_str.append(std::to_string(qv));
    qv_str.append(", ");
  }
  std::chrono::steady_clock::time_point qvend = std::chrono::steady_clock::now();
  qv_str.pop_back();
  qv_str.pop_back();
  qv_str.append("]");

  std::cout << "quantiles here: qv=" << qv_str << std::endl;
  std::cout << "Ingest/add time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms], " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
  std::cout << "quantile value time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(qvend - qvbegin).count() << "[ms], " << std::chrono::duration_cast<std::chrono::milliseconds>(qvend - qvbegin).count() << "[µs]" << std::endl;

}


string check_qvs(DDsketch& sk) {
  std::vector<double> qvs {0.5, 0.75, 0.9, 0.95, 0.99, 1.0};
  std::string qv_str = "[";
  for (auto& e : qvs) {
    double qv = sk.get_quantile_value(e);
    qv_str.append(std::to_string(qv));
    qv_str.append(", ");
  }
  qv_str.pop_back();
  qv_str.pop_back();
  qv_str.append("]");
  return qv_str;

}

string check_qvs(ExactQuantiles& eq) {
  std::vector<double> qvs {0.5, 0.75, 0.9, 0.95, 0.99, 1.0};
  std::string qv_str = "[";
  for (auto& e : qvs) {
    double qv = eq.get_quantile_value(e);
    qv_str.append(std::to_string(qv));
    qv_str.append(", ");
  }
  qv_str.pop_back();
  qv_str.pop_back();
  qv_str.append("]");
  return qv_str;

}
void simple_merge_test1() {

  DDsketch sk1 = DDsketch();
  DDsketch sk2 = DDsketch();

  std::vector<double> values {        0.82237959,       -0.61125007,       0.4383297,       0.63498452,       1.30493411};
  std::vector<double> values2 { 0.82237959 + 1.0, -0.61125007 - 1.0, 0.4383297 + 1.0, 0.63498452 + 1.0, 1.30493411 - 1.0};
  for (size_t e = 0; e < values.size(); e++) {
    std::cout << " sk1 adding elem: " << values[e] << "\n";
    sk1.add(values[e]);
    sk2.add(values2[e]);
    std::cout << " sk1 adding done of elem: " << values[e] << "\n";
  }
  string qv_str_1 = check_qvs(sk1);
  string qv_str_2 = check_qvs(sk2);

  std::cout << "quantiles here: \nqv1=" << qv_str_1 << "\nqv2=" << qv_str_2 << std::endl;
  ///*
  sk1.merge(sk2);
  string qv_str_3 = check_qvs(sk1);
  std::cout << "merged quantiles here: \nqv3=" << qv_str_3 << std::endl;
  //*/
}

// tests merging of two datasets
void baseline3(const string& file1, const string& file2){
  DDsketch sk1 = DDsketch();
  DDsketch sk2 = DDsketch();
  std::vector<double> values1, values2;
  readfile(values1, file1);
  readfile(values2, file2);
  for (auto& e : values1) sk1.add(e);
  for (auto& e : values2) sk2.add(e);


  string qv_str_1 = check_qvs(sk1);
  string qv_str_2 = check_qvs(sk2);

  std::cout << "quantiles here: \nqv1=" << qv_str_1 << "\nqv2=" << qv_str_2 << std::endl;
  ///*
  sk1.merge(sk2);
  string qv_str_3 = check_qvs(sk1);
  std::cout << "merged quantiles here: \nqv3=" << qv_str_3 << std::endl;
}

//function called for threads to add vals to sketch
void t_add(int t_id, DDsketch* sk, vector<double> *vals_to_add, size_t start, size_t stop) {
  (void)t_id;  // thread index, kept for readability of the spawn call

  for(size_t e = start; e < stop; e++) sk->add((*vals_to_add)[e]);

}

//baseline3 but just faster, try several threads
void baseline4_threaded() {

//  std::vector<double> values1(9990); // => ??? GB in memory
//  std::vector<double> values1(599999); //=> ??? GB in memory; too fast for vtune
//  std::vector<double> values1(599999990); //=> 4.5 GB in memory
//  std::vector<double> values1(799999990); //=> 6 GB in memory
  std::vector<double> values1(999999999); //=> 7.4 GB in memory
  generate_uniform_dataset(values1);

  int num_threads = 4;
  vector<DDsketch> sketch_vec(num_threads);

  vector<std::thread> threads;
  size_t range = values1.size() / num_threads;
  size_t start_range = 0, end_range = range;
  cout << "starting threads " << endl;
  std::chrono::steady_clock::time_point begin_add = std::chrono::steady_clock::now();
  for (int e = 0; e < num_threads; e++) {
    if (e == num_threads - 1) end_range = values1.size(); // to account for oddly-sized datasets
//    cout << " thread " << e << " has workload of " << end_range - start_range  << " values" << endl;
    threads.push_back(std::thread(t_add, e, &sketch_vec[e], &values1, start_range, end_range));
    start_range += range;
    end_range += range;
  }
  for (auto & t: threads) t.join();
  std::chrono::steady_clock::time_point end_add = std::chrono::steady_clock::now();
  //merge all TODO make non-custom and simple, is already fast
  std::chrono::steady_clock::time_point begin_merge = std::chrono::steady_clock::now();
  sketch_vec[0].merge(sketch_vec[1]);
  sketch_vec[2].merge(sketch_vec[3]);
  sketch_vec[0].merge(sketch_vec[2]);
  std::chrono::steady_clock::time_point end_merge = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point qvbegin = std::chrono::steady_clock::now();
  string qv_str_3 = check_qvs(sketch_vec[0]);
  std::chrono::steady_clock::time_point qvend = std::chrono::steady_clock::now();

  std::cout << "merged quantiles here: \nqv3=" << qv_str_3 << std::endl;
  std::cout << "Ingest/ADD time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_add - begin_add).count() << "[ms], " << std::chrono::duration_cast<std::chrono::microseconds>(end_add - begin_add).count() << "[µs]" << std::endl;
  std::cout << "Ingest/MERGE time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_merge - begin_merge).count() << "[ms], " << std::chrono::duration_cast<std::chrono::microseconds>(end_merge - begin_merge).count() << "[µs]" << std::endl;
  std::cout << "quantile value time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(qvend - qvbegin).count() << "[ms], " << std::chrono::duration_cast<std::chrono::milliseconds>(qvend - qvbegin).count() << "[µs]" << std::endl;
}

//compares calculated quantiles between ddsketch and ExactQuantiles
void cmd_ddsketch_eq() {

  std::vector<double> values1(999990);
  generate_uniform_dataset(values1);
  ExactQuantiles eq  = ExactQuantiles();
  DDsketch sk1 = DDsketch();
  for (auto& elem: values1) {
    sk1.add(elem);
    eq.add(elem);
  }
  eq.sort();
  string qv_str_dd = check_qvs(sk1);
  string qv_str_eq = check_qvs(eq);

  cout << "ddsketch:: " << qv_str_dd << "\n exactq:: " << qv_str_eq;

}

//test get_quantile_value for ExactQuantiles
void eq_qv() {

  std::vector<double> values1{1.0, 2.0, 3.0, 4.0, 5.0}; // => ??? GB in memory
  ExactQuantiles eq  = ExactQuantiles();
  for(auto & e : values1) eq.add(e);
  eq.sort();
  cout << "eq-qv=" << eq.get_quantile_value(0.5) << endl;
}


void test_exact_quantiles_performance(vector<double> &ds, double gb, int test_run){
  std::vector<double> qvs {0.5, 0.75, 0.9, 0.95, 0.99, 1.0};
  std::vector<double> results(qvs.size());
  ExactQuantiles eq  = ExactQuantiles();
  std::chrono::steady_clock::time_point add_start = std::chrono::steady_clock::now();
  for(auto & e : ds) eq.add(e);
  std::chrono::steady_clock::time_point add_end = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point sort_start = std::chrono::steady_clock::now();
  eq.sort();
  std::chrono::steady_clock::time_point sort_end = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point get_qv_start = std::chrono::steady_clock::now();
  for (size_t e = 0; e < qvs.size(); e++) results[e] = eq.get_quantile_value(qvs[e]);
  std::chrono::steady_clock::time_point get_qv_end = std::chrono::steady_clock::now();
  cout <<  "alg=sbeq run=" << test_run << " size=" << gb
   << " t_add_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(add_end - add_start).count()
   << " t_add_s="  << std::chrono::duration_cast<std::chrono::seconds>(add_end - add_start).count()
    << " t_sort_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(sort_end - sort_start).count()
    << " t_sort_s="  << std::chrono::duration_cast<std::chrono::seconds>(sort_end - sort_start).count()
    << " t_get_qv_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(get_qv_end - get_qv_start).count()
    << " t_get_qv_s="  << std::chrono::duration_cast<std::chrono::seconds>(get_qv_end - get_qv_start).count();
  for(size_t e = 0; e< results.size(); e++){
    cout << " qv" << qvs[e] << "=" << results[e];
  }
  cout << endl;

}

/* uncommented because it did not work in the end
void test_sb_quantiles_performance(vector<double> &ds, double gb, int test_run){
  int UniverseBits = 30;
  int seed = 0;
//  SbQuantiles h(UniverseBits, 0.5);
   uint32_t U_size = TreeNode::u_size(UniverseBits);
    std::mt19937 gen;
    gen.seed(seed);
    std::uniform_int_distribution<> dis(0, U_size-1);
  int items = 50000;
  std::chrono::steady_clock::time_point add_start = std::chrono::steady_clock::now();
  for(int i = 0; i < items; i++) {
  int x = dis(gen);
  h.insert(x);
  
  }
  std::chrono::steady_clock::time_point add_end = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point sort_start = std::chrono::steady_clock::now();
  h.compress_from_root();
  std::chrono::steady_clock::time_point sort_end = std::chrono::steady_clock::now();
  
  cout <<  "alg=sbeq run=" << test_run << " size=" << gb
   << " t_add_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(add_end - add_start).count()
   << " t_add_s="  << std::chrono::duration_cast<std::chrono::seconds>(add_end - add_start).count()
    << " t_sort_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(sort_end - sort_start).count()
    << " t_sort_s="  << std::chrono::duration_cast<std::chrono::seconds>(sort_end - sort_start).count();
  cout << endl;

}

void test_exact_quantiles_performance_with_int(vector<double> &ds, double gb, int test_run){
    int seed = 0;
int  UniverseBits = 30;
      std::mt19937 gen;
    gen.seed(seed);
       uint32_t U_size = TreeNode::u_size(UniverseBits);
    std::uniform_int_distribution<> dis(0, U_size-1);
  int items = 50000;
  
  
  ExactQuantiles eq  = ExactQuantiles();
  std::chrono::steady_clock::time_point add_start = std::chrono::steady_clock::now();
  for(auto & e : ds) eq.add(e);
  std::chrono::steady_clock::time_point add_end = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point sort_start = std::chrono::steady_clock::now();
  eq.sort();
  std::chrono::steady_clock::time_point sort_end = std::chrono::steady_clock::now();
  
  cout <<  "alg=sbeq run=" << test_run << " size=" << gb
   << " t_add_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(add_end - add_start).count()
   << " t_add_s="  << std::chrono::duration_cast<std::chrono::seconds>(add_end - add_start).count()
    << " t_sort_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(sort_end - sort_start).count()
    << " t_sort_s="  << std::chrono::duration_cast<std::chrono::seconds>(sort_end - sort_start).count();
  
  cout << endl;

}
*/
void test_ddsketch_performance(int num_threads, int test_run, vector<double> &ds, double gb){
  std::vector<double> qvs {0.5, 0.75, 0.9, 0.95, 0.99, 1.0};
  std::vector<double> results(qvs.size());
  vector<DDsketch> sketch_vec(num_threads);
  vector<std::thread> threads;
  size_t range = ds.size() / num_threads;
  size_t start_range = 0, end_range = range;

  std::chrono::steady_clock::time_point add_start = std::chrono::steady_clock::now();
  if (num_threads == 1){
    for (size_t e = 0; e < ds.size(); e++) {
      sketch_vec[0].add(ds[e]);
    }
  } else{
    for (int e = 0; e < num_threads; e++) {
      if (e == num_threads - 1) end_range = ds.size(); // to account for oddly-sized datasets
      //    cout << " thread " << e << " has workload of " << end_range - start_range  << " values" << endl;
      threads.push_back(std::thread(t_add, e, &sketch_vec[e], &ds, start_range, end_range));
      start_range += range;
      end_range += range;
    }
    for (auto & t: threads) t.join();
  }
  std::chrono::steady_clock::time_point add_end = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point merge_start = std::chrono::steady_clock::now();
  if (num_threads != 1){ // no merge necessary in single-threaded
    for(int e = 1; e < num_threads; e++){
      sketch_vec[0].merge(sketch_vec[e]);
    }
  }
  std::chrono::steady_clock::time_point merge_end = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point get_qv_start = std::chrono::steady_clock::now();
  for (size_t e = 0; e < qvs.size(); e++) results[e] = sketch_vec[0].get_quantile_value(qvs[e]);
  std::chrono::steady_clock::time_point get_qv_end = std::chrono::steady_clock::now();

  cout <<  "alg=dd run=" << test_run << " size=" << gb << " num_threads=" << num_threads
       << " space=" << sketch_vec[0].total_arr_size()
       << " t_add_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(add_end - add_start).count()
       << " t_add_s="  << std::chrono::duration_cast<std::chrono::seconds>(add_end - add_start).count()
       << " t_merge_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(merge_end - merge_start).count()
       << " t_merge_s="  << std::chrono::duration_cast<std::chrono::seconds>(merge_end - merge_start).count()
       << " t_get_qv_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(get_qv_end - get_qv_start).count()
       << " t_get_qv_s="  << std::chrono::duration_cast<std::chrono::seconds>(get_qv_end - get_qv_start).count();
  for(size_t e = 0; e< results.size(); e++){
    cout << " qv" << qvs[e] << "=" << results[e];
  }
  cout << endl;
}



// benchmarks the fixed-width HistogramSketch (single-threaded ingest)
// benchmarks the fixed-width HistogramSketch; the bin range is derived from the
// data so it works on any distribution (synthetic or a loaded real dataset).
void test_histogram_performance(int test_run, vector<double> &ds, double gb,
                                double lo, double hi){
  std::vector<double> qvs {0.5, 0.75, 0.9, 0.95, 0.99, 1.0};
  std::vector<double> results(qvs.size());
  HistogramSketch hist(lo, hi, 100000);

  auto add_start = std::chrono::steady_clock::now();
  for (size_t e = 0; e < ds.size(); e++) hist.add(ds[e]);
  auto add_end = std::chrono::steady_clock::now();
  auto get_qv_start = std::chrono::steady_clock::now();
  for (size_t e = 0; e < qvs.size(); e++) results[e] = hist.get_quantile_value(qvs[e]);
  auto get_qv_end = std::chrono::steady_clock::now();

  cout << "alg=hist run=" << test_run << " size=" << gb
       << " space=" << hist.total_arr_size()
       << " t_add_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(add_end - add_start).count()
       << " t_add_s="   << std::chrono::duration_cast<std::chrono::seconds>(add_end - add_start).count()
       << " t_get_qv_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(get_qv_end - get_qv_start).count()
       << " t_get_qv_s="  << std::chrono::duration_cast<std::chrono::seconds>(get_qv_end - get_qv_start).count();
  for (size_t e = 0; e < results.size(); e++) cout << " qv" << qvs[e] << "=" << results[e];
  cout << endl;
}

// benchmarks the merging t-digest (single-threaded ingest)
void test_tdigest_performance(int test_run, vector<double> &ds, double gb){
  std::vector<double> qvs {0.5, 0.75, 0.9, 0.95, 0.99, 1.0};
  std::vector<double> results(qvs.size());
  TDigest td(100.0);

  auto add_start = std::chrono::steady_clock::now();
  for (size_t e = 0; e < ds.size(); e++) td.add(ds[e]);
  auto add_end = std::chrono::steady_clock::now();
  auto get_qv_start = std::chrono::steady_clock::now();
  for (size_t e = 0; e < qvs.size(); e++) results[e] = td.get_quantile_value(qvs[e]);
  auto get_qv_end = std::chrono::steady_clock::now();

  cout << "alg=tdigest run=" << test_run << " size=" << gb
       << " space=" << td.total_arr_size()
       << " t_add_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(add_end - add_start).count()
       << " t_add_s="   << std::chrono::duration_cast<std::chrono::seconds>(add_end - add_start).count()
       << " t_get_qv_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(get_qv_end - get_qv_start).count()
       << " t_get_qv_s="  << std::chrono::duration_cast<std::chrono::seconds>(get_qv_end - get_qv_start).count();
  for (size_t e = 0; e < results.size(); e++) cout << " qv" << qvs[e] << "=" << results[e];
  cout << endl;
}

// benchmarks the KLL sketch (single-threaded ingest)
void test_kll_performance(int test_run, vector<double> &ds, double gb){
  std::vector<double> qvs {0.5, 0.75, 0.9, 0.95, 0.99, 1.0};
  std::vector<double> results(qvs.size());
  KLL kll(256);

  auto add_start = std::chrono::steady_clock::now();
  for (size_t e = 0; e < ds.size(); e++) kll.add(ds[e]);
  auto add_end = std::chrono::steady_clock::now();
  auto get_qv_start = std::chrono::steady_clock::now();
  for (size_t e = 0; e < qvs.size(); e++) results[e] = kll.get_quantile_value(qvs[e]);
  auto get_qv_end = std::chrono::steady_clock::now();

  cout << "alg=kll run=" << test_run << " size=" << gb
       << " space=" << kll.total_arr_size()
       << " t_add_ms="  << std::chrono::duration_cast<std::chrono::milliseconds>(add_end - add_start).count()
       << " t_add_s="   << std::chrono::duration_cast<std::chrono::seconds>(add_end - add_start).count()
       << " t_get_qv_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(get_qv_end - get_qv_start).count()
       << " t_get_qv_s="  << std::chrono::duration_cast<std::chrono::seconds>(get_qv_end - get_qv_start).count();
  for (size_t e = 0; e < results.size(); e++) cout << " qv" << qvs[e] << "=" << results[e];
  cout << endl;
}

// runs all algorithms over a single in-memory dataset, `runs` times each.
void run_one_dataset(vector<double>& ds, double size_label, int runs) {
  if (ds.empty()) { cout << "empty dataset, skipping" << endl; return; }
  // histogram range derived from the data (guard against constant data)
  double lo = *std::min_element(ds.begin(), ds.end());
  double hi = *std::max_element(ds.begin(), ds.end());
  if (!(hi > lo)) hi = lo + 1.0;

  vector<int> threads_to_use{1, 2, 4, 8};
  for (int test_run = 1; test_run <= runs; test_run++) {
    for (auto& tcount : threads_to_use) test_ddsketch_performance(tcount, test_run, ds, size_label);
    test_exact_quantiles_performance(ds, size_label, test_run);
    test_histogram_performance(test_run, ds, size_label, lo, hi);
    test_tdigest_performance(test_run, ds, size_label);
    test_kll_performance(test_run, ds, size_label);
  }
}

//the main testing function: benchmarks DDsketch, ExactQuantiles, the
//HistogramSketch and the t-digest across dataset sizes, thread counts and runs.
// dataset_gbs defaults to {0.25, 0.5}; pass sizes on the command line to override.
// runs: how many times each configuration is repeated (the plotter averages them).
// NOTE: >6 GB needs a lot of RAM (the dataset plus the exact-quantile copy).
void full_testrun_dd_eq(std::vector<double> dataset_gbs = {0.25, 0.5},
                        int runs = 9,
                        Distribution dist = Distribution::Uniform) {

  size_t a_gig_of_vals = 125000000; //sizeof(double): 8bytes, so that is a gig
  cout << "distribution=" << distribution_name(dist) << " runs_per_config=" << runs << endl;

  for (size_t e = 0; e < dataset_gbs.size(); e++) {
    vector<double> ds(static_cast<size_t>(dataset_gbs[e] * a_gig_of_vals));
    generate_dataset(ds, dist);
    cout << "generated dataset w/ size=" << dataset_gbs[e] << " Gb" << endl;
    run_one_dataset(ds, dataset_gbs[e], runs);
  }
  }
  
 /* TODO implement
  void full_testrun_dd_eq_sb_with_int() {

  //TODO create testing loop, testing various amounts of gb of data both on ExactQuantiles and DDsketches.
  //do 9 runs each, average it out. Also record space requirements using vec.size() * sizeof(double)
  size_t a_gig_of_vals = 125000000; //sizeof(double): 8bytes, so that is a gig
  // testing 0.25, 0.5, 1, 2 , 4, 8, 12 gb
//  std::vector<double> dataset_gbs{0.25, 0.5, 1, 2, 4, 8, 12}; // 12 is a bit ambitious, eq would the need to  have 24 gbs to store both in memory.
  std::vector<double> dataset_gbs{0.25, 0.5, 1, 2, 3, 4, 5, 6, 7, 8}; // full sweep
  //TODO more than 6 gb breaks a 16-gb system
//  std::vector<double> dataset_gbs{0.25}; // quick test
  std::vector<size_t> dataset_sizes; //
  for (auto & elem : dataset_gbs) {
    dataset_sizes.push_back(elem *  a_gig_of_vals);
    cout << " newest elem  of datasetvec=" << dataset_sizes.back() <<  endl;
  }

  // go over all dataset sizes
  for (int e = 0; e< dataset_sizes.size(); e++) {
    vector<double> ds(dataset_sizes[e]);
    generate_uniform_dataset(ds);
    cout << "generated dataset w/ size=" << dataset_gbs[e] << " Gb" << endl;
    //TODO run it 9 times each!!
    int test_run = 1;
    // evaluate both  eq and ddsketch
    //ddsketch first, with several amounts of threads
//    vector<int>  threads_to_use{1, 2, 4, 8};  //TODO reenable



    // exact quantiles now
    cout << "go into eq method" << endl;
    test_exact_quantiles_performance_with_int(ds, dataset_gbs[e], test_run);
    cout << "go into sb method" << endl;
    test_sb_quantiles_performance(ds, dataset_gbs[e], test_run);
    
    
  }
  
}
*/
//  test to dry-run the merging pattern
// super complicated, that could've been a while loop in retrospect
void test_merge_pattern(int threads){
  cout << " merging "<< threads << " threads" << endl;
  vector<vector<int>> rounds(1 + log2(threads) );
  rounds[0] = vector<int> (threads);
  std::iota(rounds[0].begin(), rounds[0].end(), 0);
  for (auto& e :rounds[0]) cout << "elem of r0  " << e;
  cout << endl;
  for(size_t e = 1; e < rounds.size(); e++){
    rounds[e].resize(rounds[e-1].size() / 2);

    cout << " re=" << e << " siziing down from " << rounds[e-1].size() << " to " << rounds[e].size() << endl;
    // fill newly resized vector now
    for(size_t k = 0; k < rounds[e].size(); k++){
    rounds[e][k] = rounds[e-1][k*2];
    }
    cout << "re="<< e;
    for(size_t j = 0;  j  < rounds[e].size(); j++) {
       cout <<  " " << rounds[e][j];
    }
    cout << endl;
  }
  if (rounds[rounds.size()-1].size() != 1) cout << "MERGE PATTERN BROKEN, REDO IT" << endl;
  cout << "all threads are done correctly" << endl;
}

void  dataset_generation_example_integer() {
//  size_t a_gig_of_vals = 125000000; //sizeof(double): 8bytes, so that is a gigabyte of data
  size_t a_gig_of_vals_int = 125000000 * 2; //sizeof(int32): 4bytes, so that is a gigabyte of data
  std::vector<double> dataset_gbs{0.25, 0.5, 1, 2, 3, 4, 5, 6, 7, 8}; // TODO adjust for testing & your system overall
  //any more and eq does not work on my 16gb system
  std::vector<size_t> dataset_sizes; //
  cout << " Dataset sizes which are generated: [";
  for (auto & elem : dataset_gbs) {
    dataset_sizes.push_back(elem *  a_gig_of_vals_int);
    cout << elem << ", ";
  }
  cout << "]" << endl;
  // TODO this creates a dataset with sizes from 0.25 gb to 8 Gb, it makes sense to call your experiments from here.
  for (size_t e = 0; e < dataset_sizes.size(); e++) {
    vector<int> ds(dataset_sizes[e]); // dataset is in this vector
    generate_uniform_dataset_integer(ds);
  }
}

int main(int argc, char** argv) {
    std::cout << "Hello from main!\n";
    // CLI:  ./Data_Summaries [sizes_in_GB...] [--runs=N]
    //                        [--dist=uniform|normal|exp|lognormal] [--file=PATH]
    //   e.g. ./Data_Summaries 0.25 0.5 --runs=9 --dist=lognormal
    //        ./Data_Summaries --file=latencies.txt --runs=5   (real dataset)
    std::vector<double> sizes;
    int runs = 9;
    Distribution dist = Distribution::Uniform;
    std::string file;
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg.rfind("--runs=", 0) == 0) {
        runs = std::stoi(arg.substr(7));
      } else if (arg.rfind("--file=", 0) == 0) {
        file = arg.substr(7);
      } else if (arg.rfind("--dist=", 0) == 0) {
        std::string d = arg.substr(7);
        if (d == "normal") dist = Distribution::Normal;
        else if (d == "exp" || d == "exponential") dist = Distribution::Exponential;
        else if (d == "lognormal" || d == "latency") dist = Distribution::Lognormal;
        else dist = Distribution::Uniform;
      } else {
        sizes.push_back(std::stod(arg));  // positional: dataset size in GB
      }
    }

    if (!file.empty()) {
      // benchmark a real dataset loaded from disk
      std::vector<double> ds = load_dataset_file(file);
      double size_label = ds.size() / 125000000.0;  // GB-equivalent for the x-axis
      cout << "distribution=file runs_per_config=" << runs << endl;
      run_one_dataset(ds, size_label, runs);
    } else if (sizes.empty()) {
      full_testrun_dd_eq({0.25, 0.5}, runs, dist);
    } else {
      full_testrun_dd_eq(sizes, runs, dist);
    }
  return 0;
}

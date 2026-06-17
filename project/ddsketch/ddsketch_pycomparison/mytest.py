from ddsketch import DDSketch
import numpy as np
from timeit import default_timer as timer
print("creating my sketch")


# basic test of add and get_qv
def baseline1():
  sketch = DDSketch()
  values = [0.82237959, -0.61125007, 0.4383297, 0.63498452, 1.30493411]
  print("MAIN:values entered: {}".format(values))
  for v in values:
    print("MAIN: adding value {}  to sketch".format(v))
    sketch.add(v)
    print("Main-printsketch: {}".format(sketch))

  quantiles = [sketch.get_quantile_value(q) for q in [0.5, 0.75, 0.9, 0.95, 0.99, 1.0]]
  print("Quantiles here: {}".format(quantiles))


def readfile(file):
  with open(file, "r") as f:
    file_in_mem = f.read()
  removed_brackets = file_in_mem[1:]  # slice off leading '['
  removed_brackets = removed_brackets[:-1]  # slice off trailing ']'
  tokenized = removed_brackets.split(" ")
  for e in range(0, len(tokenized)):
    tokenized[e] = float(tokenized[e].strip())

  return tokenized


# read from file, test add and get_qv
def baseline2(file):

  values = readfile(file)
  print("file ingest done")
  sketch = DDSketch()
  #add list to ddsketch
  b4_ts = timer()
  for v in values:
    # print("MAIN: adding value {}  to sketch".format(v))
    sketch.add(v)
    # print("Main-printsketch: {}".format(sketch))
  after_ts = timer()
  print("Ingest/add Time taken: {}".format(after_ts - b4_ts))
  print(" adding done")
  #eval the common quantiles
  qv_start_ts = timer()
  quantiles = [sketch.get_quantile_value(q) for q in [0.5, 0.75, 0.9, 0.95, 0.99, 1.0]]
  qv_end_ts = timer()
  print("Quantiles here: {}".format(quantiles))
  print("Quantile value Time taken: {}".format(qv_end_ts - qv_start_ts))

def baseline3(file, file2):
  values1 = readfile(file)
  values2 = readfile(file2)
  print("file ingest done")
  sk1 = DDSketch()
  sk2 = DDSketch()
  #add list to ddsketch
  b4_ts = timer()
  for v in values1:
    sk1.add(v)
  for v in values2:
    sk2.add(v)
  after_ts = timer()
  print("Time taken: {}".format(after_ts - b4_ts))
  print(" adding done")
  #eval the common quantiles
  qv1 = [sk1.get_quantile_value(q) for q in [0.5, 0.75, 0.9, 0.95, 0.99, 1.0]]
  qv2 = [sk2.get_quantile_value(q) for q in [0.5, 0.75, 0.9, 0.95, 0.99, 1.0]]
  print("Quantiles here: qv1={}\nqv2={}".format(qv1, qv2))
  sk1.merge(sk2)
  qv3 = [sk1.get_quantile_value(q) for q in [0.5, 0.75, 0.9, 0.95, 0.99, 1.0]]
  print("merged Quantiles here: qv3={}".format(qv3))


def simple_merge_test1():
  sk1 = DDSketch()
  sk2 = DDSketch()
  values = [0.82237959, -0.61125007, 0.4383297, 0.63498452, 1.30493411]
  values2 = [0.82237959 + 1.0 , -0.61125007 - 1.0, 0.4383297 + 1.0, 0.63498452 + 1.0, 1.30493411 - 1.0]

  for e in range(len(values)):
    sk1.add(values[e])
    sk2.add(values2[e])

  qv1 = [sk1.get_quantile_value(q) for q in [0.5, 0.75, 0.9, 0.95, 0.99, 1.0]]
  qv2 = [sk2.get_quantile_value(q) for q in [0.5, 0.75, 0.9, 0.95, 0.99, 1.0]]
  print("Quantiles here: \nsk1={} \n sk2={}".format(qv1, qv2))
  sk1.merge(sk2)
  qv3 = [sk1.get_quantile_value(q) for q in [0.5, 0.75, 0.9, 0.95, 0.99, 1.0]]
  print("merged Quantiles here: \nsk3={} ".format(qv3))

def main():
  import sys
  # pass a dataset file to benchmark ingest (baseline2); otherwise run the
  # small in-memory sanity check (baseline1, no file needed).
  if len(sys.argv) > 1:
    baseline2(sys.argv[1])
  else:
    baseline1()


if __name__ == '__main__':
  main()

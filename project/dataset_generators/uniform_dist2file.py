import numpy as np
import time

def my_arr2str(arr):
    result = "["
    for elem in arr:
        # result  += str(elem)
        result  += "{:.5f}".format(elem)
        result += " "

    result = result[:-1]
    result += "]"
    return result

# generates a uniform distribution and saves it comma-separated to a file
def main():
    # TODO adjust values as needed, then just hit run. The dataset will be named according to these hyperparameters.
    min_val = -1
    max_val = 1
    sample_size = 90000000
    filename = "uniform_dist_[" + str(min_val) + "," + str(max_val) + "[_" + str(sample_size) + "_samples.txt"
    s = np.random.uniform(min_val, max_val, sample_size)
    result = my_arr2str(s)  # exchanged np.arr2string, which ran endlessly after a small amount of samples

    # write to file
    f = open(filename, "w")
    f.write(result)
    f.close()
    print(" written to file '{}'".format(filename))


if __name__ == '__main__':
    main()

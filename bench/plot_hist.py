#!/usr/bin/env python

import sys
import numpy as np
import matplotlib.pyplot as plt
import math


def read_array(path):
    file = open(path, 'r')
    lines = file.readlines()
    array = []
    for line in lines:
        array.append(float(line))
    return np.array(array)


def round_up(n, decimals=0):
    multiplier = 10 ** decimals
    return math.ceil(n * multiplier) / multiplier


def plot_hist(size, cpu, gpu):
    cpu_array = read_array(cpu)
    gpu_array = read_array(gpu)

    if size == 1:
        title = "1 поток"
    else:
        title = str(size) + " потоков"

    lim = round_up(math.sqrt(size) / 20, 2)
    bins = np.linspace(0, lim, 100)
    cpu_label = "CPU ({})".format("i7-7700K CPU @ 4.20GHz")
    gpu_label = "GPU ({})".format("AMD Vega 56")
    plt.hist(x=cpu_array, bins=bins,
             color='blue', label=cpu_label,
             alpha=0.5, rwidth=0.85)
    plt.hist(x=gpu_array, bins=bins,
             color='red', label=gpu_label,
             alpha=0.5, rwidth=0.85)

    plt.grid(axis='y', alpha=0.75)
    plt.xlabel('Время вычисления, секунд')
    plt.ylabel('')
    plt.legend(loc='upper right')
    plt.title(title)
    #plt.xlim(0, lim)
    #maxfreq = n.max()
    # Set a clean upper y-axis limit.
    #plt.ylim(ymax=np.ceil(maxfreq / 10) * 10 if maxfreq % 10 else maxfreq + 10)
    plt.savefig('plots/{}.svg'.format(size))


if __name__ == "__main__":
    plot_hist(int(sys.argv[1]), sys.argv[2], sys.argv[3])

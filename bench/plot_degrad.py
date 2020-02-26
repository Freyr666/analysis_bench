#!/usr/bin/env python

import sys
import numpy as np
import matplotlib.pyplot as plt
import math


#cpu_name = "i7-7700K CPU @ 4.20GHz"
#gpu_name = "AMD Vega 56"
cpu_name = "i3-7100U CPU @ 2.40GHz"
gpu_name = "Intel HD Graphics 620"


def read_array(path):
    file = open(path, 'r')
    lines = file.readlines()
    array = []
    for line in lines:
        array.append(float(line))
    return np.array(array)


def read_mean(prefix, name, begin, end):
    mean = []
    std = []
    for i in range(begin, end+1):
        a = read_array("{}/{}{}".format(prefix, name, i))
        mean.append(a.mean())
        std.append(a.std())
    return (np.array(mean), np.array(std))


def plot_degrad(prefix, begin, end):
    x = np.arange(begin, end+1, 1)
    mean_cpu, std_cpu = read_mean(prefix, 'cpu', begin, end)
    mean_gpu, std_gpu = read_mean(prefix, 'gpu', begin, end)

    cpu_title = "CPU ({})".format(cpu_name)
    gpu_title = "GPU ({})".format(gpu_name)

    plt.xlabel('Количество программ')
    plt.ylabel('Время вычисления, секунд')
    #plt.title(title)
    plt.errorbar(x, np.array(mean_cpu), yerr=np.array(std_cpu), fmt='-o', label=cpu_title)
    plt.errorbar(x, np.array(mean_gpu), yerr=np.array(std_gpu), fmt='-o', color='red', label=gpu_title)
    plt.legend(loc='upper right')
    plt.savefig("{}/plots/degrad.png".format(prefix))


if __name__ == "__main__":
    plot_degrad(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]))

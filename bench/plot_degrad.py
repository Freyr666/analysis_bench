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


def plot_degrad(prefix, begin, end):
    mean = []
    std = []

    for i in range(begin, end+1):
        a = read_array("data/{}{}".format(prefix, i))
        mean.append(a.mean())
        std.append(a.std())

    x = np.arange(begin, end+1, 1)
    mean = np.array(mean)
    std = np.array(std)

    if prefix == 'cpu':
        title = "CPU ({})".format("i7-7700K CPU @ 4.20GHz")
    else:
        title = "GPU ({})".format("AMD Vega 56")

    plt.xlabel('Количество программ')
    plt.ylabel('Время вычисления, секунд')
    plt.title(title)
    plt.errorbar(x, np.array(mean), yerr=np.array(std), fmt='-o')
    plt.savefig("plots/degrad_{}.svg".format(prefix))


if __name__ == "__main__":
    plot_degrad(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]))

#!/usr/bin/env python
import argparse
import os
import re
import locale

from matplotlib import rc
import matplotlib.pyplot as plt
import numpy as np

dot_line_formats = {
    'std_mutex': ('bs', '-b'),
    'tbb_srw_mutex': ('rd', '-r'),
    'tbb_qrw_mutex': ('ro', '-r'),
    'rcuptr': ('gv', '-g'),
    'rcuptr_jss': ('g^', '-g'),
    'urcu': ('c+', '-c'),
    'urcu_mb': ('cx', '-c'),
}


class Measure:
    def __init__(self, test_bin, read_op, vec_size, num_readers, num_writers):
        self.test_bin = test_bin
        self.read_op = read_op
        self.vec_size = vec_size
        self.num_readers = num_readers
        self.num_writers = num_writers

        self.reader_sum = -1


class ChartLine:
    def __init__(self):
        self.values = dict()
        self.x = []
        self.y = []

    def __str__(self):
        return "x: " + str(self.x) + "\n" + "y: " + str(self.y)


def display(measures, vec_size, num_writers, read_op, value, skip_urcu=False):
    chartData = dict()
    for m in measures:
        # if int(m.num_readers) > 5:
            # continue
        if skip_urcu and 'urcu' in m.test_bin:
            continue
        if (m.vec_size == vec_size and m.num_writers ==
                num_writers and m.read_op == read_op):
            if m.test_bin not in chartData:
                chartData[m.test_bin] = ChartLine()
            chartData[m.test_bin].values[int(m.num_readers)] = getattr(m, value)

    for key, chartline in chartData.iteritems():
        lists = sorted(chartline.values.items())
        chartline.x, chartline.y = zip(*lists)

    plt.title(
        read_op +
        " vec_size: " +
        str(vec_size) +
        " num_writers: " +
        str(num_writers))
    plt.ylabel(value)
    plt.xlabel("# reader threads")
    plotChartData(chartData)


def plotChartData(chartData):
    for key, chartline in chartData.iteritems():
        plot(chartline.x, chartline.y, key[len("measure_"):])
    plt.show()


def plot(xs, ys, name):
    # interpolation
    # fit = np.polyfit(xs, ys, 1)
    # fit_fn = np.poly1d(fit)
    # plt.plot(xs, fit_fn(xs), line_formats[i])

    ax = plt.subplot(111)

    # linear scale
    # ax.plot(xs, ys, dot_line_formats[name][0], label=name)
    # ax.plot(xs, ys, dot_line_formats[name][1])
    # ax.legend(loc='upper left', shadow=True, fontsize='small')

    # logarithmic scale
    ax.semilogy(xs, ys, dot_line_formats[name][0], label=name)
    ax.semilogy(xs, ys, dot_line_formats[name][1])
    ax.legend(loc='lower right', fontsize='small', shadow=True, ncol=2)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--result_dir', help='path of result dir',
                        required=True)
    parser.add_argument('--skip_urcu', dest='skip_urcu', action='store_true')
    parser.set_defaults(skip_urcu=False)
    args = parser.parse_args()

    patterns = [
        (re.compile("reader sum: ([\d|\.]+)"), 'reader_sum'),
        (re.compile("writer sum: ([\d|\.]+)"), 'writer_sum'),
        (re.compile("reader av: ([\d|\.]+)"), 'reader_av'),
        (re.compile("writer av: ([\d|\.]+)"), 'writer_av'),
    ]

    measures = []
    for file in os.listdir(args.result_dir):
        if '__' not in file:
            continue
        elements = file.split('__')
        measure = Measure(elements[0], elements[1], elements[2],
                          elements[3], elements[4])
        for _, line in enumerate(open(os.path.join(args.result_dir, file))):
            for pattern, attr in patterns:
                for match in re.finditer(pattern, line):
                    value = match.groups()[0]
                    locale.setlocale(locale.LC_NUMERIC, '')
                    setattr(measure, attr, int(locale.atof(value)))
        measures.append(measure)

    # display(measures, '1', '1', 'read_all', 'reader_sum', args.skip_urcu)
    # display(measures, '1024', '1', 'read_all', 'reader_sum', args.skip_urcu)
    # display(measures, '8196', '1', 'read_all', 'reader_sum', args.skip_urcu)
    # display(measures, '131072', '1', 'read_all', 'reader_sum', args.skip_urcu)
    # display(measures, '1048576', '1', 'read_all', 'reader_sum', args.skip_urcu)

    display(measures, '8196', '2', 'read_all', 'reader_sum', args.skip_urcu)
    display(measures, '131072', '2', 'read_all', 'reader_sum', args.skip_urcu)
    display(measures, '1048576', '2', 'read_all', 'reader_sum', args.skip_urcu)


if __name__ == "__main__":
    main()

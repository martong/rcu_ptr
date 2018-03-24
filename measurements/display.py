#!/usr/bin/env python
import argparse
import os
import re
import locale

from matplotlib import rc
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

dot_line_formats = {
    'std_mutex': ('bs', '-b'),
    'tbb_srw_mutex': ('rd', '-r'),
    'tbb_qrw_mutex': ('ro', '-r'),
    'rcuptr': ('gv', '-g'),
    'rcuptr_jss': ('g^', '-g'),
    'urcu': ('c*', '-c'),
    'urcu_mb': ('c+', '-c'),
    'urcu_bp': ('cx', '-c'),
}


class Measure:
    def __init__(
            self,
            test_bin,
            vec_size,
            num_all_readers,
            num_readers,
            num_writers):
        self.test_bin = test_bin
        self.num_all_readers = num_all_readers
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


def display(
        measures,
        vec_size,
        num_writers,
        num_all_readers,
        value,
        args):
    chartData = dict()
    for m in measures:
        # if int(m.num_readers) > 5:
            # continue
        if args.skip_urcu and 'urcu' in m.test_bin:
            continue
        if args.skip_mtx and 'mutex' in m.test_bin:
            continue
        if (m.vec_size == vec_size and m.num_writers ==
                num_writers and m.num_all_readers == num_all_readers):
            if m.test_bin not in chartData:
                chartData[m.test_bin] = ChartLine()
            chartData[m.test_bin].values[int(m.num_readers)] = getattr(m, value)

    for key, chartline in chartData.iteritems():
        lists = sorted(chartline.values.items())
        chartline.x, chartline.y = zip(*lists)

    title = " vec_size: " + str(vec_size) + " num_writers: " + str(
        num_writers) + " num_all_readers: " + str(num_all_readers)
    title = title.replace('_', ' ')
    plt.title(title)
    plt.ylabel(value.replace('_', ' '))
    plt.xlabel("reader threads")
    for key, chartline in chartData.iteritems():
        plot(chartline.x, chartline.y, key[len("measure_"):], args)

    if args.save:
        filename = "_".join(
            ["res", str(vec_size),
             str(num_all_readers),
             str(num_writers)])
        if args.latex:
            plt.savefig(filename + ".eps", format='eps', dpi=1000)
        else:
            plt.savefig(filename + ".png")
        plt.clf()
    else:
        plt.show()


def plot(xs, ys, name, args):
    ax = plt.subplot(111)

    # logarithmic scale
    if args.log:
        ax.semilogy(xs, ys, dot_line_formats[name][0],
                    label=name.replace('_', ' '))
        ax.semilogy(xs, ys, dot_line_formats[name][1])
        ax.legend(loc='lower right', fontsize='small', shadow=True, ncol=2)

    # linear scale
    else:
        ax.plot(xs, ys, dot_line_formats[name][0], label=name.replace('_', ' '))
        ax.plot(xs, ys, dot_line_formats[name][1])
        ax.legend(loc='upper left', shadow=True, fontsize='small')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--result_dir', help='path of result dir',
                        required=True)
    parser.add_argument('--skip_urcu', action='store_true')
    parser.add_argument('--skip_mtx', action='store_true')
    parser.add_argument('--latex', action='store_true')
    parser.add_argument('--save', action='store_true')
    parser.add_argument('--log', action='store_true')
    args = parser.parse_args()

    if args.latex:
        rc('text', usetex=True)
        rc('font', **{'family': 'serif', 'serif': ['Computer Modern Roman']})
        matplotlib.rcParams.update({'font.size': 10})

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

    display(measures, '8196', '1', '0', 'writer_sum', args)
    display(measures, '131072', '1', '0', 'writer_sum', args)
    display(measures, '1048576', '1', '0', 'writer_sum', args)

    # slow readers too
    # display(measures, '8196', '1', '1', 'reader_sum', args)
    # display(measures, '131072', '1', '1', 'reader_sum', args)
    # display(measures, '1048576', '1', '1', 'reader_sum', args)

    # slow readers too
    # display(measures, '8196', '1', '1', 'writer_sum', args)
    # display(measures, '131072', '1', '1', 'writer_sum', args)
    # display(measures, '1048576', '1', '1', 'writer_sum', args)


if __name__ == "__main__":
    main()

#!/usr/bin/env python
import argparse
import os
import re
import locale

from matplotlib import rc
import matplotlib
import matplotlib.pyplot as plt

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


# Represents one measurement configuration.
class MeasureKey:

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

    def __str__(self):
        return (self.test_bin +
                " " +
                str(self.vec_size) +
                " " +
                str(self.num_all_readers) +
                " " +
                str(self.num_readers) +
                " " +
                str(self.num_writers))

    def __hash__(self):
        return self.__str__().__hash__()

    def __eq__(self, other):
        return self.__str__().__eq__(other.__str__())


# We have multiple measurement values for the same configuration (i.e.
# MeasureKey).
class MeasureIterations:

    def __init__(self):
        self.reader_sum = []
        self.writer_sum = []

    def __str__(self):
        return str(self.reader_sum)


class ChartLine:

    def __init__(self):
        self.values = dict()
        self.x = []
        self.y = []

    def __str__(self):
        return "x: " + str(self.x) + "\n" + "y: " + str(self.y)


# param l: list
def getAverage(l):
    l.remove(max(l))
    l.remove(min(l))
    return sum(l) / len(l)


# Get a more verbose better reading label to a value
def getYlabel(value):
    m = {'reader_sum': 'Number of Read Operations / second',
         'writer_sum': 'Number of Write Operations / second'}
    return m[value]


def display(
        measures,
        vec_size,
        num_writers,
        num_all_readers,
        args):
    chartData = dict()
    value = args.value
    for measureKey, measureIterations in measures.iteritems():
        print(measureKey)
        print(measureIterations)
        print("=======================")
        if args.skip_urcu and 'urcu' in measureKey.test_bin:
            continue
        if args.skip_mtx and 'mutex' in measureKey.test_bin:
            continue
        if (measureKey.vec_size == vec_size and measureKey.num_writers ==
                num_writers and measureKey.num_all_readers == num_all_readers):
            if measureKey.test_bin not in chartData:
                chartData[measureKey.test_bin] = ChartLine()
            chartData[
                measureKey.test_bin].values[
                int(measureKey.num_readers)] = getAverage(
                getattr(measureIterations, value))

    for key, chartline in chartData.iteritems():
        lists = sorted(chartline.values.items())
        chartline.x, chartline.y = zip(*lists)

    title = " vec_size: " + str(vec_size) + " num_writers: " + str(
        num_writers) + " num_all_readers: " + str(num_all_readers)
    title = title.replace('_', ' ')
    #plt.title(title)
    #plt.ylabel(value.replace('_', ' '))
    plt.ylabel(getYlabel(value))
    plt.xlabel("Number of Reader Threads")
    for key, chartline in chartData.iteritems():
        plot(chartline.x, chartline.y, key[len("measure_"):], args)

    if args.save:
        filename = "_".join(
            ["res", str(value), str(vec_size),
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

    fig_loc = 'lower right'
    if args.fig_loc is not None:
        fig_loc = args.fig_loc

    # logarithmic scale
    if args.log:
        ax.semilogy(xs, ys, dot_line_formats[name][0],
                    label=name.replace('_', ' '))
        ax.semilogy(xs, ys, dot_line_formats[name][1])
        ax.legend(loc=fig_loc, fontsize='small', shadow=True, ncol=2)

    # linear scale
    else:
        ax.plot(xs, ys, dot_line_formats[name][0], label=name.replace('_', ' '))
        ax.plot(xs, ys, dot_line_formats[name][1])
        ax.legend(loc=fig_loc, shadow=True, fontsize='small')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--result_dir', help='path of result dir',
                        required=True)
    parser.add_argument('--value', required=True, choices=['reader_sum', 'writer_sum'])
    parser.add_argument('--skip_urcu', action='store_true')
    parser.add_argument('--skip_mtx', action='store_true')
    parser.add_argument('--latex', action='store_true')
    parser.add_argument('--save', action='store_true')
    parser.add_argument('--log', action='store_true')
    parser.add_argument('--fig_loc', default=None)
    args = parser.parse_args()

    if args.latex:
        rc('text', usetex=True)
        rc('font', **{'family': 'serif', 'serif': ['Computer Modern Roman']})
        matplotlib.rcParams.update({'font.size': 15})

    patterns = [
        (re.compile("reader sum: ([\d|\.]+)"), 'reader_sum'),
        (re.compile("writer sum: ([\d|\.]+)"), 'writer_sum'),
    ]

    # Aggregate the values in each files into a dict, where the key is
    # MeasureKey, and the value is MeasureIterations.
    # We append the found values in each measureIteration to the specific list.
    measures = dict()
    for file in os.listdir(args.result_dir):
        basename = os.path.splitext(file)[0]
        elements = basename.split('__')
        key = MeasureKey(elements[0], elements[1], elements[2],
                         elements[3], elements[4])
        if key not in measures:
            measures[key] = MeasureIterations()
        measureIt = measures[key]
        for _, line in enumerate(open(os.path.join(args.result_dir, file))):
            for pattern, attr in patterns:
                for match in re.finditer(pattern, line):
                    value = match.groups()[0]
                    locale.setlocale(locale.LC_NUMERIC, '')
                    values = getattr(measureIt, attr)
                    values.append(int(locale.atof(value)))
                    setattr(measureIt, attr, values)

    # slow readers too
    """
    display(measures, '8196', '1', '1', args)
    display(measures, '131072', '1', '1', args)
    display(measures, '1048576', '1', '1', args)
    """

    # no slow readers
    display(measures, '8196', '1', '0', args)
    #display(measures, '131072', '1', '0', args)
    #display(measures, '1048576', '1', '0', args)


if __name__ == "__main__":
    main()

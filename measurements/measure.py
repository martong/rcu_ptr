#!/usr/bin/env python
import subprocess
import argparse
import shutil
import os
import multiprocessing


def call_command(cmd, cwd=None, env=None):
    """
    Execute a process in a test case.  If the run is successful do not bloat
    the test output, but in case of any failure dump stdout and stderr.
    Returns the utf decoded (stdout, stderr) pair of strings.
    """
    def show(out, err):
        print("\nTEST execute stdout:\n")
        print(out.decode("utf-8"))
        print("\nTEST execute stderr:\n")
        print(err.decode("utf-8"))
    try:
        proc = subprocess.Popen(cmd,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                cwd=cwd, env=env)
        out, err = proc.communicate()
        if proc.returncode != 0:
            show(out, err)
            print('Unsuccessful run: "' + ' '.join(cmd) + '"')
            raise Exception("Unsuccessful run of command.")
        return out.decode("utf-8"), err.decode("utf-8")
    except OSError:
        show(out, err)
        print('Failed to run: "' + ' '.join(cmd) + '"')
        raise


def one_measure(
        args,
        test_bin,
        num_all_readers,
        vec_size,
        num_writers,
        num_readers,
        iteration):
    binary = os.path.join(args.bin_dir, test_bin)
    file_name = '__'.join([test_bin, vec_size, num_all_readers, num_readers,
                           num_writers])
    file_name = file_name + "." + str(iteration)
    print(file_name)
    out, err = call_command(
        ['perf', 'stat', '-d', binary, vec_size, num_all_readers, num_readers,
         num_writers])
    with open(os.path.join(args.result_dir, file_name), 'w') as f:
        f.write(out)
        f.write(err)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bin_dir', help='path to the test binary',
                        required=True)
    parser.add_argument('--result_dir', help='path of result dir',
                        required=True)
    args = parser.parse_args()

    if os.path.exists(args.result_dir):
        shutil.rmtree(args.result_dir)
    os.mkdir(args.result_dir)

    test_bins = [
        "measure_std_mutex",
        "measure_rcuptr",
        "measure_rcuptr_jss",
        "measure_tbb_qrw_mutex",
        "measure_tbb_srw_mutex",
        "measure_urcu_bp",
    ]

    vec_sizes = ['8196', '131072', '1048576']
    all_readers = ['0', '1']
    writers = ['1']
    measure_iterations = 5

    print("cpu count: " + str(multiprocessing.cpu_count()))
    for iteration in range(0, measure_iterations):
        for test_bin in test_bins:
            for num_all_readers in all_readers:
                for vec_size in vec_sizes:
                    for num_writers in writers:
                        max_readers = (
                            multiprocessing.cpu_count() -
                            int(num_writers) -
                            int(num_all_readers))
                        for num_readers in range(1, max_readers + 1):
                            one_measure(
                                args,
                                test_bin,
                                num_all_readers,
                                vec_size,
                                num_writers,
                                str(num_readers),
                                iteration
                            )


if __name__ == "__main__":
    main()

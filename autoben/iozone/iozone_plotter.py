#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Plots IOZone result.

It uses matplotlib to plot IOZone result of device(s). One plot per benchmark.
For each plot, it has subplots grouped by primary key (either file size
or record length).
"""

import argparse
import logging
import os

import matplotlib.pyplot as plt

import iozone_parser


def PlotIozone(device_matrix, benchmark, group_by, primary_keys, dest_dir):
  """Plots IOZone result for specific benchmark and writes file.

  It writes to <dest_dir>/<benchmark>.png.

  Args:
    device_matrix: list of (device name, its iozone matrix)
    benchmark: benchmark to plot.
    group_by: subplot group by either "reclen" or "filesize".
    primary_keys: list of primary keys
    dest_dir: destination directory.
  """
  COLOR_CODE = ['b', 'r', 'g', 'c', 'm', 'y', 'k']
  NUM_ROW = 4
  NUM_COL = 4

  def InitPlot():
    axes = [None] * NUM_ROW * NUM_COL
    fig = plt.figure(figsize=(20, 20))
    fig.suptitle(benchmark, fontsize=20)
    fig.subplots_adjust(wspace=0.5, hspace=0.5)

    # Draw subplot x/y axes.
    if group_by == 'filesize':
      title = 'file size (kB): %d'
      xlabel = 'record lenght (kB)'
    else:
      title = 'record lenght (kB): %d'
      xlabel = 'file size (kB)'
    for subplot_index, primary_key in enumerate(primary_keys):
      ax = fig.add_subplot(NUM_ROW, NUM_COL, subplot_index + 1, sharey=axes[0])
      ax.set_title(title % primary_key)
      ax.set_xlabel(xlabel)
      ax.set_xscale('log')
      ax.set_ylabel('speed (bytes/sec)')
      axes[subplot_index] = ax
    return fig, axes

  # Sanity check.
  if len(primary_keys) > NUM_ROW * NUM_COL:
    logging.error('#primary keys exceed #row x #col')
    return

  fig, axes = InitPlot()

  # Plot for all devices.
  for ith_device, (device, matrix) in enumerate(device_matrix):
    for subplot_index, primary_key in enumerate(primary_keys):
      benchmark_result = matrix.Result(benchmark, group_by)
      if primary_key not in benchmark_result:
        continue
      xy_values = benchmark_result[primary_key]
      x_values, y_values = zip(*xy_values)
      axes[subplot_index].plot(x_values, y_values, COLOR_CODE[ith_device],
                               label=device)

  handles, labels = axes[0].get_legend_handles_labels()
  fig.legend(handles, labels, 'upper right')
  fig.savefig(os.path.join(dest_dir, '%s.png' % benchmark))


def _UnionPrimaryKeys(device_matrix, group_by):
  """Gets the union of primary keys among devices' result matrix.

  Args:
    device_matrix: list of (device_name, result_matrix)
    group_by: type of primary key

  Returns:
    List of sorted primary keys.
  """
  primary_keys = set()
  for _, matrix in device_matrix:
    if group_by == 'filesize':
      primary_keys.update(matrix.file_sizes)
    else:
      primary_keys.update(matrix.reclens)
  return sorted(primary_keys)


def main():
  parser = argparse.ArgumentParser(description='Parse IOZone output file.')
  parser.add_argument('infiles', type=str, nargs='+', metavar='infile',
                      help='IOZone output file(s)')
  parser.add_argument('--group-by', default='reclen', dest='group_by',
                      help='group by either "reclen"(default) or "filesize"')
  parser.add_argument(
      '--benchmark',
      help='a comma separated benchmarks to plot. Possible values: %r' % (
          iozone_parser.IOZONE_BENCHMARKS))
  parser.add_argument('--dest', default='./',
                      help='destination directory')
  args = parser.parse_args()

  logging.basicConfig()

  # Parse iozone output per device.
  device_matrix = []
  for infile_name in args.infiles:
    iozone_result = iozone_parser.IozoneResult(infile_name)
    device_matrix.append((iozone_result.device_name, iozone_result))

  primary_keys = _UnionPrimaryKeys(device_matrix, args.group_by)
  # Plot iozone result per benchmark.
  if args.benchmark:
    benchmarks = args.benchmark.split(',')
    valid_benchmarks = set(iozone_parser.IOZONE_BENCHMARKS)
    for b in benchmarks:
      if b not in valid_benchmarks:
        print 'Invalid benchmark: ' + b
        print 'Valid benchmarks: %s' % iozone_parser.IOZONE_BENCHMARKS
        return
  else:
    benchmarks = iozone_parser.IOZONE_BENCHMARKS

  for benchmark in benchmarks:
    PlotIozone(device_matrix, benchmark, args.group_by, primary_keys, args.dest)


if __name__ == '__main__':
  main()

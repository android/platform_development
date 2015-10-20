#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Measures I/O read/write speed using dd."""

import argparse
import logging
import os
import re

import adb


class DiskPerformanceDd(object):
  """Measure disk I/O performance using dd."""

  _RE_THROUGHPUT = re.compile(
        r'\d+ bytes transferred in \d+.\d+ secs \((\d+) bytes/sec\)')

  # For print_level.
  BRIEF = 1
  DETAIL = 2

  def __init__(self, target, serial_number=None, verbose=False,
               print_level=None, drop_cache=False, num_files=1,
               average=False):
    """Constructor.

    Args:
      target: Target directory or block device to read/write.
      serial_number: If specified, connect to the device.
      verbose: True to print out command.
      print_level: If specified (BRIEF or DETAIL), also print the result.
      drop_cache: True to drop cache.
      num_files: Number of files to read/write.
      average: True to output average throughtput of all files.
    """
    self._adb = adb.AdbCommand(serial_number=serial_number,
                               verbose=verbose)
    self._verbose = verbose
    self._print_level = print_level
    self._block_mode = target.startswith('/dev/block')
    self._target = target if self._block_mode else os.path.join(target,
                                                                'randomfile')
    self._drop_cache = drop_cache
    self._num_files = num_files
    self._average = average
    if self._block_mode:
      self._num_files = 1
      self._average = False

  def _GetThroughput(self, dd_result, bs, read_write):
    """Gets throughput from dd result and maybe print it out."""
    if self._verbose:
      logging.info(dd_result)

    m = self._RE_THROUGHPUT.search(dd_result)
    if m:
      throughput = int(m.group(1))
    else:
      logging.warning('Cannot get throughput from dd result: %s', dd_result)
      throughput = None

    # If there's no need to average multiple files' throughput, print out
    # throughput once available.
    if not self._average:
      self._PrintThroughput(throughput, bs, read_write)

    return throughput

  def _PrintThroughput(self, throughput, bs, read_write):
    if not self._print_level:
      return

    if throughput is None:
      print '%s failed (bs %s)' % (read_write, bs)
    elif self._print_level == self.BRIEF:
      print throughput
    else:
      print '%s (bs %d): %d bytes/sec' % (read_write, bs, throughput)

  def MayDropCache(self):
    if self._drop_cache:
      self._adb.Run('sync')
      self._adb.Run('echo 3 > /proc/sys/vm/drop_caches')

  @staticmethod
  def _GetRealTime(time_output):
    """Gets 'real' field from 'time -p' output.

    Returns:
      The value of 'real' in float. 0 if not found.
    """
    m = re.search(r'^real\s+(\d+\.\d+)', time_output, re.MULTILINE)
    if not m:
      return 0
    return float(m.group(1))

  def GetIthFilename(self, ith):
    """Gets the i-th filename for read/write.

    Args:
      ith: i-th file.

    Return:
      Path of the i-th file.
    """
    if self._num_files == 1:
      return self._target
    return '%s.%d_%d' % (self._target, ith, self._num_files)

  def Write(self, size, bs=1048576, duplicate=False):
    """Writes #size megabytes of random things to file.

    dd if=/dev/urandom of=/path/to/target/file bs=1048576 count=100

    Args:
      size: file size in MB.
      bs: block size (bytes).
      duplicate: True to duplicate the first file instead from /dev/urandom.

    Return:
      List of file(s) write throughput (bytes/sec).
      None if dd fails to get throughput.
    """
    if self._block_mode:
      logging.error("Block mode doesn't support write")
      return
    count = 1048576 * size / bs
    result = []
    for ith in xrange(self._num_files):
      self.MayDropCache()
      if duplicate and ith > 0:
        ifile = self.GetIthFilename(0)
      else:
        ifile = '/dev/urandom'
      ofile = self.GetIthFilename(ith)
      dd_result = self._adb.Run(
          'time -p dd if=%s of=%s bs=%d count=%d' % (
              ifile, ofile, bs, count))
      sync_result = self._adb.Run('time -p sync')
      if self._print_level == self.DETAIL:
        time_used = self._GetRealTime(dd_result) + self._GetRealTime(
            sync_result)
        print 'Elapsed time: %.2f seconds' % time_used
      result.append(self._GetThroughput(dd_result, bs, 'Write'))

    if self._average:
      average_throughput = float(sum(result)) / len(result)
      self._PrintThroughput(average_throughput, bs, 'Write')
    return result

  def Read(self, size, bs):
    """Measures read speed.

    dd if=/path/to/target/file of=/dev/null bs=8k

    Args:
      size: size to read in MB.
      bs: block size.

    Return:
      List of file(s) read throughput (bytes/sec).
      None if dd fails to get throughput.
    """
    self.MayDropCache()
    result = []
    count_param = ''
    if self._block_mode:
      count_param = ' count=%d' % (1048576 * size / bs)

    for ith in xrange(self._num_files):
      dd_result = self._adb.Run(
          'time -p dd if=%s of=/dev/null bs=%d%s' % (
              self.GetIthFilename(ith), bs, count_param))
      if self._print_level == self.DETAIL:
        time_used = self._GetRealTime(dd_result)
        print 'Elapsed time: %.2f seconds' % time_used
      result.append(self._GetThroughput(dd_result, bs, 'Read'))

    if self._average:
      average_throughput = float(sum(result)) / len(result)
      self._PrintThroughput(average_throughput, bs, 'Read')
    return result

  def SweepRead(self, size, start_bs, end_bs):
    """Measures read speed using different block size.

    It tests bs from start_bs to end_bs, steps: x2.

    Args:
      start_bs: start block size.
      end_bs: end block size.

    Returns:
      List of (bs, throughput). Throughput might be None if dd fails.
    """
    bs = start_bs
    result = []
    while bs <= end_bs:
      result.append((bs, self.Read(size, bs)))
      bs *= 2
    return result


if __name__ == '__main__':
  parser = argparse.ArgumentParser(
      description='Measures I/O read/write speed using dd.',
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('--verbose', action='store_true',
                      help='print out verbose messgae')
  parser.add_argument('--device', help='Specify adb device serial number')
  parser.add_argument('--write', action='store_true',
                      help='write a random file')
  parser.add_argument('--read', action='store_true',
                      help='read a random file')
  parser.add_argument(
      '--drop_cache', action='store_true',
      help='Drop cache using "echo 3 > /proc/sys/vm/drop_caches"')
  parser.add_argument(
      '--dir', default='/data/local/tmp',
      help=('working directory to read/write a random file or block device if '
            'it starts with /dev/block'))
  parser.add_argument(
      '--num_files', type=int, default=1,
      help='if specify, it writes/reads file from dir/randomfile.#n')
  parser.add_argument(
      '--average', action='store_true',
      help='if specify, output average throughput of all files')
  parser.add_argument(
      '--duplicate', action='store_true',
      help='for multiple files write, duplicate the first one')
  parser.add_argument(
      '--size', type=int, default=100,
      help='size of a file to write or a block device to read (MB)')
  parser.add_argument('--start_bs', type=int, default=4096,
                      help='start block size for read-sweeping (bytes)')
  parser.add_argument('--end_bs', type=int, default=1048576,
                      help='end block size for read-sweeping (bytes)')
  parser.add_argument('--brief', action='store_true',
                      help='only print out throughtput (bytes/sec) number(s)')
  args = parser.parse_args()

  logging.basicConfig(format='%(asctime)s:%(levelname)s: %(message)s',
                      level=logging.DEBUG if args.verbose else logging.INFO)

  print_level = (DiskPerformanceDd.BRIEF if args.brief else
                 DiskPerformanceDd.DETAIL)

  disk_perf = DiskPerformanceDd(
      args.dir, verbose=args.verbose, serial_number=args.device,
      print_level=print_level, drop_cache=args.drop_cache,
      num_files=args.num_files, average=args.average)

  if args.write:
    disk_perf.Write(args.size, duplicate=args.duplicate)
  elif args.read:
    disk_perf.SweepRead(args.size, args.start_bs, args.end_bs)
  else:
    parser.print_help()

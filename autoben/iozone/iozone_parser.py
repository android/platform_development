# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Methods to parse iozone output file.

iozone: http://www.iozone.org/
"""

import collections
import logging
import os
import re

IOZONE_BENCHMARKS = ['write', 'rewrite', 'read', 'reread', 'random read',
                     'random write', 'bkwd read', 'record rewrite',
                     'stride read',  'fwrite', 'frewrite', 'fread', 'freread']

class BenchmarkResult(object):
  """IOZone result for a benchmark.

  It normalizes result into two groups: by_file_size and by_reclen.
  """
  def __init__(self):
    self.by_file_size = collections.defaultdict(list)
    self.by_reclen = collections.defaultdict(list)


class IozoneResult(object):
  """Parses IOZone output and normailzes it in different way.

  You can get device name, list of file size and reclen (record length) of the
  result (see property device_name, file_sizes, reclens).
  You can get result of a benchmark grouped by primary key using
  Result(benchmark, group_by)
  """
  def __init__(self, filename):
    self._matrix = RetrieveIozoneMatrix(filename)
    self._device_name = os.path.basename(filename)
    self._file_sizes = []
    self._reclens = []
    self._benchmark_result = collections.defaultdict(BenchmarkResult)
    self._ParseMatrix()

  def _ParseMatrix(self):
    file_size_set = set()
    reclen_set = set()
    for fields in self._matrix:
      file_size, reclen = fields[:2]
      file_size_set.add(file_size)
      reclen_set.add(reclen)

      benchmarks = fields[2:]
      if len(benchmarks) != len(IOZONE_BENCHMARKS):
        logging.warning('Invalid fields: %s', fields)
        break

      for benchmark_name, value in zip(IOZONE_BENCHMARKS, benchmarks):
        result = self._benchmark_result[benchmark_name]
        result.by_file_size[file_size].append((reclen, value))
        result.by_reclen[reclen].append((file_size, value))

    self._file_sizes = sorted(file_size_set)
    self._reclens = sorted(reclen_set)

  def Result(self, benchmark, group_by):
    """Gets result of a benchmark grouped by group_by.

    Args:
      benchmark: benchmark to retrieve result.
      group_by: either 'reclen' or 'filesize'.

    Returns:
      {primary key: list of (secondary key, result)}, where primary key
      is specified by group_by, and secondary key is the other key.
    """
    if group_by == 'reclen':
      return self._benchmark_result[benchmark].by_reclen
    else:
      return self._benchmark_result[benchmark].by_file_size

  @property
  def file_sizes(self):
    return self._file_sizes

  @property
  def reclens(self):
    return self._reclens

  @property
  def device_name(self):
    return self._device_name


def RetrieveIozoneMatrix(filename):
  """Retrieves IOZone result matrix.

  It skips header description and puts results in a list of list.

  Args:
    filename: input file.

  Returns:
    A list of IOZone output [filesize, reclen, measured benchmarks...].
  """
  header_re = re.compile('kB\s+reclen\s+write')
  result = []

  start_matrix = False
  expected_num_fields = len(IOZONE_BENCHMARKS) + 2
  for line in open(filename):
    if start_matrix:
      fields = line.strip().split()
      if not fields:
        break
      if len(fields) != expected_num_fields:
        logging.warning('Invalid input %s', line.strip())
      try:
        fields = [int(x) for x in fields]
        result.append(fields)
      except ValueError:
        logging.warning('Invalid input %s', line.strip())
    else:
      if header_re.search(line):
        start_matrix = True

  return result

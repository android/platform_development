#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Sets Android device's CPU frequency from host machine."""

import argparse
import logging
import re
import sys

import adb


class CpuFrequency(object):
  """Sets/gets CPU frequency."""
  def __init__(self, serial_number=None, verbose=False):
    """Constructor.

    Args:
      serial_number: If specified, connect to the device.
      verbose: True to print out command.
    """
    self._adb = adb.AdbCommand(serial_number=serial_number,
                               verbose=verbose)
    self._verbose = verbose
    self._sys_cpu_base = '/sys/devices/system/cpu'

    # Use property to access them (lazy initialization).
    self._cpufreq_paths = None
    self._available_frequencies = None
    self._max_freq = None
    self._min_freq = None

  @property
  def cpufreq_paths(self):
    if not self._cpufreq_paths:
      paths = self._adb.Run('ls -d %s/cpu?/cpufreq' % self._sys_cpu_base)
      self._cpufreq_paths = [p.strip() for p in paths.split('\n') if p]
    return self._cpufreq_paths

  @property
  def available_frequencies(self):
    if not self._available_frequencies:
      frequencies = self._adb.Run(
          'cat %s/scaling_available_frequencies' % self.cpufreq_paths[0])
      self._available_frequencies = [int(f) for f in frequencies.split() if f]
    return self._available_frequencies

  @property
  def max_freq(self):
    if not self._max_freq:
      self._max_freq = max(self.available_frequencies)
    return self._max_freq

  @property
  def min_freq(self):
    if not self._min_freq:
      self._min_freq = min(self.available_frequencies)
    return self._min_freq

  def GetCurrentFrequencies(self):
    """Gets current CPUs' frequencies.

    Returns:
      List of (cpu_id, frequency)
    """
    command = ['cat']
    for path in self.cpufreq_paths:
      command.append('%s/cpuinfo_cur_freq' % path)
    command_result = self._adb.Run(' '.join(command))
    frequencies = command_result.split('\n')
    re_cpu_id = re.compile('cpu/(cpu\d)')
    result = []
    for i, path in enumerate(self.cpufreq_paths):
      m = re_cpu_id.search(path)
      cpu_id = m.group(1)
      result.append((cpu_id, int(frequencies[i].strip())))
    return result

  def PrintInfo(self):
    """Prints out CPU scaling governor and frequency info.
    """
    cpu_path = self.cpufreq_paths[0]
    governor = self._adb.Run('cat %s/scaling_governor' % cpu_path).strip()
    max_freq = self._adb.Run('cat %s/scaling_max_freq' % cpu_path).strip()
    min_freq = self._adb.Run('cat %s/scaling_min_freq' % cpu_path).strip()
    print 'CPU governor: %s  max_freq: %s  min_freq: %s' % (
        governor, max_freq, min_freq)

    print 'CPU frequency: %s' % ' '.join(
        '%s: %d' % id_freq for id_freq in self.GetCurrentFrequencies())

  def SetMaxFrequency(self):
    """Sets CPU frequency to maximum."""
    self.SetFrequency(self.max_freq)

  def SetGovernor(self, governor):
    """Sets CPU scaling governor."""
    for path in self.cpufreq_paths:
      self._adb.Run('echo %s > %s/scaling_governor' % (governor, path))

  def SetScalingRange(self, min_freq, max_freq):
    """Sets CPU scaling governor's minimum and maximum frequency."""
    for path in self.cpufreq_paths:
      self._adb.Run('echo %d > %s/scaling_min_freq' % (min_freq, path))
      self._adb.Run('echo %d > %s/scaling_max_freq' % (max_freq, path))

  def SetFrequency(self, freq):
    """Sets CPU frequncy."""
    if freq not in self.available_frequencies:
      logging.error('Invalid frequency %d. Available frequency: %s', freq,
                    ' '.join(str(f) for f in self.available_frequencies))
      return

    self.SetGovernor('userspace')
    self.SetScalingRange(freq, freq)
    for path in self.cpufreq_paths:
      self._adb.Run('echo %d > %s/scaling_setspeed' % (freq, path))

  def SetPerformance(self):
    """Sets CPU governor to performance."""
    self.SetScalingRange(self.min_freq, self.max_freq)
    self.SetGovernor('performance')

  def SetInteractive(self):
    """Sets CPU governor to interactive, which is default."""
    self.SetScalingRange(self.min_freq, self.max_freq)
    self.SetGovernor('interactive')

  def SetDefault(self):
    """Sets CPU governor to interactive, which is default."""
    self.SetInteractive()


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Get/Set CPU frequency.')
  parser.add_argument('--verbose', action='store_true',
                      help='print out verbose messgae')
  parser.add_argument('--device', help='specify adb device serial number')
  parser.add_argument('--max', action='store_true',
                      help='set CPU frequency to maximum')
  parser.add_argument('--performance', action='store_true',
                      help='set CPU governor to performance')
  parser.add_argument('--default', action='store_true',
                      help='set CPU governor to interactive')
  # args.info is unused now. No argument implies show info.
  parser.add_argument('--info', action='store_true',
                      help='print CPU frequency/governor info (default)')
  parser.add_argument('--freq', type=int,
                      help='set CPU frequency')

  has_args = len(sys.argv) > 1
  args = parser.parse_args()

  logging.basicConfig(format='%(asctime)s:%(levelname)s: %(message)s',
                      level=logging.DEBUG if args.verbose else logging.INFO)

  cpu = CpuFrequency(verbose=args.verbose, serial_number=args.device)

  if args.max:
    print 'Set CPU frequency to maximum'
    cpu.SetMaxFrequency()
  elif args.performance:
    print 'Set CPU governor to "performance"'
    cpu.SetPerformance()
  elif args.default:
    print 'Set CPU governor to "interactive"'
    cpu.SetDefault()
  elif args.freq:
    print 'Set CPU frequency to %d' % args.freq
    cpu.SetFrequency(args.freq)

  # Show info after action.
  cpu.PrintInfo()

  if not has_args:
    print '\n./cpu_freq.py -h for help.'

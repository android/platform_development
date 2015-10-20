#!/usr/bin/python
#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Memory usage monitor of specified process running on Android.

See `./memmon.py -h` for command line options.
"""
import argparse
import bisect
import time

import matplotlib.pyplot as plt

from utils import adb as adb_module


class RealTimeMonitor(object):
  """Records values and draws latest N seconds time series."""

  def __init__(self, fig_names=None, window=60):
    """Sets pyplot to interactive mode and initialises history.

    Args:
      fig_names: Names of figures to be created.
          If no names are given, create a single figure with no name.
      window: Window size in seconds.
    """
    if not fig_names:
      self.sole_fig = True
      fig_names = ("",)
    else:
      self.sole_fig = False

    plt.ion()
    self.fig_ids = dict()
    for name in fig_names:
      # Multiple RealTimeMonitor instances may co-exist, each holding different
      # figure IDs.
      fig = plt.figure()
      self.fig_ids[name] = fig.number
      plt.suptitle(name)
      plt.hold(False)
      plt.show()
    self.window = window
    # Only keeps latest `window` seconds of history.
    self.history_time = []
    self.history_value = {name: [] for name in fig_names}
    self.max_value = {name: 0 for name in fig_names}
    self.fig_names = fig_names

  def peak_window(self):
    """Gets peak value inside the window.

    Returns:
      A dict mapping figure names to corresponding peak values.
      If no names were given, return the peak value.
    """
    if self.sole_fig:
      return max(self.history_value[""])
    return {name: max(values) for name, values in self.history_value.items()}

  def peak(self):
    """Gets peak value since beginning of monitor.

    Returns:
      A dict mapping figure names to corresponding peak values.
      If no names were given, return the peak value.
    """
    if self.sole_fig:
      return self.max_value[""]
    return self.max_value

  def update(self, values):
    """Updates plot with current values.

    Since intervals between samples are not consistent, we use current time for
    x-axis in the plot.

    Args:
      values: A dict mapping figure names to corresponding values.
          If no names were given, the current value.

    Raises:
      ValueError: Figure names does not match those given in constructor.
    """
    if self.sole_fig:
      values = {"": values}

    if set(values.keys()) != set(self.fig_names):
      raise ValueError("Figure names does not match")

    now = time.time()

    # Remove data out of window.
    index = bisect.bisect(self.history_time, now - self.window)
    # Keep the first out-of-window value so the leftmost segment won't be
    # erased.
    if index > 0:
      index -= 1
    self.history_time = self.history_time[index:]
    for name in self.fig_names:
      self.history_value[name] = self.history_value[name][index:]

    self.history_time.append(now)
    for name in self.fig_names:
      self.history_value[name].append(values[name])
      plt.figure(self.fig_ids[name])
      plt.plot(self.history_time, self.history_value[name])
      plt.axis([now - self.window, now,
                0, max(max(self.history_value[name]) * 1.1, 100000)])
      plt.draw()

      self.max_value[name] = max(self.max_value[name], values[name])

  def clear(self):
    """Clears history values, including peak value."""
    self.history_time = []
    self.history_value = {name: [] for name in self.fig_names}
    self.max_value = {name: 0 for name in self.fig_names}
    for name, fig_id in self.fig_ids.items():
      plt.figure(fig_id)
      plt.clf()
      plt.suptitle(name)
      plt.draw()


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("target_proc", help="target process name")
  parser.add_argument("--adb-path", default="adb",
                      help="path to ADB executable (defaults to 'adb')")
  parser.add_argument("--adb-args", default="",
                      help="additional arguments passing to ADB "
                      "(defaults to '')")
  args = parser.parse_args()

  target_proc = args.target_proc
  adb = adb_module.ADB(args.adb_path, args.adb_args)
  monitor = RealTimeMonitor()
  try:
    while True:
      # Find pids of processes whose name **includes** target process name
      processes = []
      for line in adb.run("ps | grep %s" % target_proc):
        fields = line.split()
        pid, name = int(fields[1]), fields[-1]
        processes.append((pid, name))

      # Caluculate numbers for each process
      total_pss_kb = 0
      for pid, name in processes:
        pss_kb = 0
        for line in adb.run("cat /proc/%s/smaps 2>/dev/null | grep Pss" % pid):
          pss_kb += int(line.split()[1])
        if pss_kb == 0:
          # Process may have disappeared, ignoring
          continue
        print "%s: %s kB" % (name, pss_kb)
        total_pss_kb += pss_kb

      # Update plot
      print "total: %s kB" % total_pss_kb
      monitor.update(total_pss_kb)
  except KeyboardInterrupt:
    # Caught Ctrl-C, print max and exit.
    pass
  print "max: %s kB" % monitor.peak()


if __name__ == "__main__":
  main()

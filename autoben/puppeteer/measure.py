#!/usr/bin/env python
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

import argparse
import csv
import fileinput
import glob
import logging
import os
import re
import shutil
import sys
import tempfile
import time

import testcase as testcase_module
import tools


def extract_metrics(metrics_dir, testcase):
  """Extracts given test case metric from metrics directory.

  Reads all files in metrics_dir.
  For .html, it's the systrace metric and the value is the file path.
  For others, reads the whole file to calculate mean, min max value.
  The metric value is in format 'mean/min/max'.

  Args:
    metrics_dir: directory to store metrics
    testcase: testcase to extract

  Returns:
    {metric_name: metric_value} of the testcase
  """
  testcase_dir = os.path.join(metrics_dir, testcase)
  metrics = {}
  for metric_filename in os.listdir(testcase_dir):
    if metric_filename.endswith('html'):
      metric_name = testcase + '_systrace'
      metrics[metric_name] = 'file:///' + os.path.join(testcase_dir, metric_filename)
    else:
      metric_file = os.path.join(testcase_dir, metric_filename)
      metric_name = testcase + '_' + metric_filename
      vals = [float(line) for line in open(metric_file, 'r')]
      metrics[metric_name] = '%f/%f/%f' % (tools.mean(vals), min(vals), max(vals))
  return metrics


def add_annotation_to_systrace(metrics_dir, testcase):
  """Processes annotation and adds to systrace.
  It processes the format of annotation and appends to the original systrace file.

  Args:
    metrics_dir: directory to store metrics
    testcase: testcase to extract
  """
  TIMESTAMP_HEADER = 'timestamp-0 (0) [000] ...1 '
  testcase_dir = os.path.join(metrics_dir, testcase)
  annotation_filename = 'annotation.json'
  systrace_filename = glob.glob(os.path.join(testcase_dir, '*.html'))[0]
  annotation_file = os.path.join(testcase_dir, annotation_filename)
  systrace_file = os.path.join(testcase_dir, systrace_filename)
  trace_array = []
  count=0
  # An annotation in systrace format:
  #   timestamp-0 (0) [000] ...1 123.8: tracing_mark_write: B|0|note1
  #   timestamp-0 (0) [000] ...1 127.3: tracing_mark_write: E
  #     |          |    |
  #   [task]     [pid][cpu]  // these headings are just required forms
  # Count is use for adding E for previous annotation
  for line in open(annotation_file, 'r'):
    m = re.search('"timestamp_ns":\s(\d+)', line)
    if m:
      timestamp = str(float(m.group(1))/1000)
    m2 = re.search('"note":\s"(.*)"', line)
    if m2:
      note = m2.group(1)
      tracing_mark_head = TIMESTAMP_HEADER + timestamp + ': tracing_mark_write: '
      if count > 0:
        trace_array.append(tracing_mark_head + 'E')
      trace_array.append(tracing_mark_head + 'B|0|' + note)
      count += 1
  #Search line and insert
  insert_after = '#              | |        |      |   ||||       |         |\n'
  inserted = False
  with tempfile.NamedTemporaryFile(mode='w') as temp_fd:
    for line in open(systrace_file, 'r'):
      temp_fd.write(line)
      if not inserted and line == insert_after:
        for trace in trace_array:
          temp_fd.write(trace + '\n')
        inserted = True
    shutil.copy(temp_fd.name, systrace_file)


def output_csv(record_metrics, record_id, output_file):
  """Updates the output table.
  It appends a new record at the end of rows, and may add column(s) if the
  record has new metric(s). For existing rows which don't have new metrics'
  value, leave blank. Same for the new record.

  Args:
    record_metrics: record to add. Format: {metric_name: metric_value}
    record_id: record ID.
    output_file: the CSV file to update.
  """
  tools.init_output_if_not_exist(output_file)

  # Read original records
  table = [row for row in csv.DictReader(open(output_file, 'r'))]

  # Update the new record with record id.
  new_row = {'record_id': record_id}
  new_row.update(record_metrics)
  table.append(new_row)

  # Arrange the metrics in lexicographic order and write to the file.
  new_metric_names = (set(table[0]) | set(new_row)) - set(['record_id'])
  updated_table_metrics = ['record_id'] + sorted(new_metric_names)
  with open(output_file, 'w') as out_file:
    csv_writer = csv.DictWriter(out_file, fieldnames=updated_table_metrics)
    csv_writer.writeheader()
    csv_writer.writerows(table)



def _compose_record_id():
  current_time = time.strftime('%y-%m-%d_%H:%M:%S')
  device_model = tools.execute_command(['adb', 'shell', 'getprop',
                                        'ro.build.version.release'])
  android_version = tools.execute_command(['adb', 'shell', 'getprop',
                                           'ro.product.model'])
  record_id = ':'.join((current_time, device_model, android_version))
  return record_id.replace(' ', '_')


def parse_argument():
  parser = argparse.ArgumentParser()
  default_dir = os.path.dirname(os.path.realpath(__file__))
  parser.add_argument('--dir' , dest='working_dir', default=default_dir,
                      help='Working directory, default: %(default)s')
  parser.add_argument('--clean', '-c', action='store_true',
                      help='clean all records')
  parser.add_argument('--verbose', '-v', action='store_true',
                      help='print debug info')
  parser.add_argument('--install', action='store_true',
                      help='install the package')
  parser.add_argument('testcase', nargs='*',
                      help='testcase to run, empty for all cases')
  return parser.parse_args()


def _compose_testcases(arg_testcase):
  testcases = []
  for testcase in arg_testcase:
    if testcase in testcase_module.CASES:
      testcases.append(testcase)
    else:
      logging.error('Invalid testcase: %s', testcase)
  if not testcases:
    testcases = testcase_module.CASES.keys()
  return testcases


def main():
  if not os.environ.get('ANDROID_SDK_PATH'):
    print 'Please set the environment variable ANDROID_SDK_PATH'
    return

  args = parse_argument()
  working_dir = args.working_dir
  output_file = os.path.join(working_dir, 'output.csv')

  logging.basicConfig(stream=sys.stdout,
                      level=logging.DEBUG if args.verbose else logging.INFO)
  if args.clean:
    tools.execute_command(['rm', '-rf', output_file,
                           os.path.join(working_dir, 'metrics/*')])
  if args.install:
    apk_path = os.path.join(
        working_dir, 'app', 'build', 'outputs', 'apk',
        'app-debug-androidTest-unaligned.apk')
    tools.execute_command(['adb', 'install', '-r', apk_path])

  testcases = _compose_testcases(args.testcase)

  record_id = _compose_record_id()
  sdcard_dir = tools.execute_command(['adb', 'shell', 'echo',
                                      '$EXTERNAL_STORAGE'])
  metrics_dir = os.path.join(working_dir, 'metrics', record_id)
  os.makedirs(metrics_dir)
  tools.adb_rmdir(os.path.join(sdcard_dir, 'metrics'))

  for testcase in testcases:
    os.makedirs(os.path.join(metrics_dir, testcase))
    testcase_module.CASES[testcase].execute(record_id)
    time.sleep(5)

  tools.execute_command(['adb', 'pull', os.path.join(sdcard_dir, 'metrics'),
                   os.path.join(working_dir, 'metrics', record_id)])

  metrics = {}
  for testcase in os.listdir(metrics_dir):
    metrics.update(extract_metrics(metrics_dir, testcase))
    add_annotation_to_systrace(metrics_dir, testcase)

  output_csv(metrics, record_id, output_file)


if __name__ == '__main__':
  main()


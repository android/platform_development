#!/usr/bin/env python
#
# Copyright 2017 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Implementation of gsi_util command 'dump'."""

import logging
import sys

from gsi_util.dumpers.dump_info_list import INFO_LIST
from gsi_util.mounters.composite_mounter import CompositeMounter


class DumpReport(object):
  """DumpReport."""

  _UNKNOWN_VALUE = '<unknown>'

  def __init__(self, dump_list):
    self._dump_list = dump_list
    self._data = {}
    self._show_unknown = False

  def set_show_unknown(self):
    self._show_unknown = True

  def append(self, key, value):
    self._data[key] = value

  @staticmethod
  def _output_dump_info(key, value):
    print '{:30}: {}'.format(key, value)

  def output(self):
    for (key, _, _) in self._dump_list:
      value = self._data.get(key)
      if not value:
        if not self._show_unknown:
          continue
        value = self._UNKNOWN_VALUE

      DumpReport._output_dump_info(key, value)


def _dump_by_dumper(report, dumper_instance, info_list):
  for info in info_list:
    dump_id = info[0]
    value = dumper_instance.dump(info[2])
    report.append(dump_id, value)


def _dump(report, dump_list, mounter):
  with mounter as file_accessor:
    # query how many different dumpers to dump
    dumper_set = set([x[1] for x in dump_list])
    for dumper in dumper_set:
      # The type of a dumper is (Class, instantation args...)
      dumper_class = dumper[0]
      dumper_args = dumper[1:]
      # Create the dumper
      with dumper_class(file_accessor, dumper_args) as dumper_instance:
        info_list_for_the_dumper = [x for x in dump_list if x[1] == dumper]
        _dump_by_dumper(report, dumper_instance, info_list_for_the_dumper)


def _make_info_list(ids):
  info_list = []
  for dump_id in ids:
    info = next(x for x in INFO_LIST if x[0] == dump_id)
    if not info:
      raise RuntimeError('Unknown info ID: "{}"'.format(dump_id))
    info_list.append(info)
  return info_list


def do_dump(args):
  logging.info('==== DUMP ====')
  logging.info('  system=%s vendor=%s', args.system, args.vendor)
  if not args.system and not args.vendor:
    sys.exit('Without system nor vendor.')

  mounter = CompositeMounter()
  if args.system:
    mounter.add('system', args.system)
  if args.vendor:
    mounter.add('vendor', args.vendor)

  logging.debug('Dump ID list: %s', args.ID)
  dump_list = _make_info_list(args.ID) if len(args.ID) else INFO_LIST

  report = DumpReport(dump_list)
  if args.show_unknown:
    report.set_show_unknown()

  _dump(report, dump_list, mounter)

  report.output()

  logging.info('==== DONE ====')


def do_list_dump(_):
  for info in INFO_LIST:
    print info[0]


def setup_command_args(parser):
  # command 'dump'
  dump_parser = parser.add_parser(
      'dump', help='dump information from given image')
  dump_parser.add_argument(
      '--system', type=str, help='system image file name, folder name or "adb"')
  dump_parser.add_argument(
      '--vendor', type=str, help='vendor image file name, folder name or "adb"')
  dump_parser.add_argument(
      '-u',
      '--show-unknown',
      action='store_true',
      help='force display dump infos which are not exist')
  dump_parser.add_argument(
      'ID',
      type=str,
      nargs='*',
      help='the dump ID to be dumped. Dump all if not given')
  dump_parser.set_defaults(func=do_dump)

  # command 'list_dump'
  list_dump_parser = parser.add_parser(
      'list_dump', help='List all possible dump IDs')
  list_dump_parser.set_defaults(func=do_list_dump)

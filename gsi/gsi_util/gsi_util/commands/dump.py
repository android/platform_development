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

import argparse
import logging
import sys

from gsi_util.dumpers.dumper import Dumper
from gsi_util.mounters.composite_mounter import CompositeMounter


class DumpReporter(object):
  """DumpReport."""

  _UNKNOWN_VALUE = '<unknown>'

  def __init__(self, dump_list):
    self._dump_list = dump_list
    self._show_unknown = False

  def set_show_unknown(self):
    self._show_unknown = True

  @staticmethod
  def _output_dump_info(key, value):
    print '{:30}: {}'.format(key, value)

  def output(self, dump_result):
    for (key, _, _) in self._dump_list:
      value = dump_result.get(key)
      if not value:
        if not self._show_unknown:
          continue
        value = self._UNKNOWN_VALUE

      self._output_dump_info(key, value)


def do_list_dump(_):
  for info in Dumper.get_all_dump_list():
    print info.id


def do_dump(args):
  logging.info('==== DUMP ====')
  logging.info('  system=%s vendor=%s', args.system, args.vendor)
  if not args.system and not args.vendor:
    sys.exit('Without system nor vendor.')

  mounter = CompositeMounter()
  if args.system:
    mounter.add_by_mount_target('system', args.system)
  if args.vendor:
    mounter.add_by_mount_target('vendor', args.vendor)

  logging.debug('Dump ID list: %s', args.ID)
  dump_list = Dumper.make_dump_list_with_ids(args.ID) if len(
      args.ID) else Dumper.get_all_dump_list()

  with mounter as file_accessor:
    dumper = Dumper(file_accessor)
    dump_result = dumper.dump(dump_list)

  reporter = DumpReporter(dump_list)
  if args.show_unknown:
    reporter.set_show_unknown()
  reporter.output(dump_result)

  logging.info('==== DONE ====')


DUMP_DESCRIPTION = """'dump' command dumps information from given image

You must assign at least one image source by SYSTEM and/or VENDOR.
Image source could be:

 adb[:SERIAL_NUM]: pull the file form the device which be connected with adb
  image file name: pull the file from the given image file, e.g. the file name
                   of a GSI.
                   If a image file is assigned to be the source of system
                   image, gsu_util will detect system-as-root automatically.
      folder name: pull the file from the given folder, e.g. the system/vendor
                   folder in a Android build out folder.

You could use command 'list_dump' to query all IDs:

    $ ./gsi_util.py list_dump

For example you could use following command to query the security patch level
in an system image file:

    $ ./gsi_util.py pull --system system.img system_security_patch_level

You there is no given ID, all information will be dumped.

Here are some other usage examples:

    $ ./gsi_util.py pull --system adb --vendor adb
    $ ./gsi_util.py pull --system system.img --show-unknown
    $ ./gsi_util.py pull --system my/out/folder/system"""


def setup_command_args(parser):
  # command 'list_dump'
  list_dump_parser = parser.add_parser(
      'list_dump', help='list all possible dump IDs')
  list_dump_parser.set_defaults(func=do_list_dump)

  # command 'dump'
  dump_parser = parser.add_parser(
      'dump',
      help='dump information from given image',
      description=DUMP_DESCRIPTION,
      formatter_class=argparse.RawTextHelpFormatter)
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

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

"""Implementation of gsi_util command 'pull'."""

import logging
import shutil
import sys

from gsi_util.mounters.composite_mounter import CompositeMounter


def do_pull(args):
  logging.info('==== PULL ====')
  logging.info('  system=%s vendor=%s', args.system, args.vendor)

  if not args.system and not args.vendor:
    sys.exit('Without system nor vendor.')

  source = args.source
  dest = args.dest if args.dest else '.'

  mounter = CompositeMounter()
  if args.system:
    mounter.add('system', args.system)
  if args.vendor:
    mounter.add('vendor', args.vendor)

  with mounter as file_accessor:
    with file_accessor.prepare_file(source) as filename:
      if not filename:
        print >> sys.stderr, 'Can not dump file: {}'.format(source)
      else:
        logging.debug('Copy %s -> %s', filename, dest)
        shutil.copy(filename, dest)

  logging.info('==== DONE ====')


def setup_command_args(parser):
  # command 'pull'
  dump_parser = parser.add_parser('pull', help='pull a file from given image')
  dump_parser.add_argument(
      '--system', type=str, help='system image file name, folder name or "adb"')
  dump_parser.add_argument(
      '--vendor', type=str, help='vendor image file name, folder name or "adb"')
  dump_parser.add_argument('source', type=str, help='file name in given image.')
  dump_parser.add_argument(
      'dest',
      type=str,
      nargs='?',
      help='a file name or directory to save the file. (default: .)')
  dump_parser.set_defaults(func=do_pull)

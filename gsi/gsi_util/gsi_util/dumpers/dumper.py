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
"""Implement dump methods and utils to dump info from a mounter."""

from gsi_util.dumpers.dump_info_list import INFO_LIST


class Dumper(object):

  def __init__(self, file_accessor):
    self._file_accessor = file_accessor

  @staticmethod
  def _dump_by_dumper(dump_result, dumper_instance, info_list):
    for info in info_list:
      value = dumper_instance.dump(info.dump_index)
      dump_result[info.id] = value

  def dump(self, dump_list):
    dump_result = {}

    # query how many different dumpers to dump
    dumper_set = set([x.dumper_create_args for x in dump_list])
    for dumper_create_args in dumper_set:
      # The type of a dumper_create_args is (Class, instantation args...)
      dumper_class = dumper_create_args[0]
      dumper_args = dumper_create_args[1:]
      # Create the dumper
      with dumper_class(self._file_accessor, dumper_args) as dumper_instance:
        info_list_for_the_dumper = (
            x for x in dump_list if x.dumper_create_args == dumper_create_args)
        self._dump_by_dumper(dump_result, dumper_instance,
                             info_list_for_the_dumper)

    return dump_result

  @staticmethod
  def make_dump_list_with_ids(ids):
    info_list = []
    for dump_id in ids:
      info = next((x for x in INFO_LIST if x.id == dump_id), None)
      if not info:
        raise RuntimeError('Unknown info ID: "{}"'.format(dump_id))
      info_list.append(info)
    return info_list

  @staticmethod
  def get_all_dump_list():
    return INFO_LIST

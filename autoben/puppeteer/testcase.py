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

import logging
import os
import re
import subprocess
import sys
import threading
import time

import tools

android_sdk_path = os.environ.get('ANDROID_SDK_PATH')

class TestCase(object):
  """Contains information about the testcase.

  Attributes:
    name (str): name of the testcase
    wait_time (int): seconds systrace waits from automator start
    trace_time (int): seconds systrace takes to trace activity
    trace_start_time (long): the nanotime of device when systrace starts
  """

  def __init__(self, name, wait_time, trace_time):
    self.name = name
    self.wait_time = wait_time
    self.trace_time = trace_time
    self.trace_start_time = -1

  def execute(self, record_id):
    logging.debug('Running %s', self.name)
    path = os.path.join('metrics', record_id, self.name, self.name + '.html')
    systrace = os.path.join(android_sdk_path, 'external/chromium-trace/systrace.py')
    command = ['python', systrace, '-b', '32000', '-t', str(self.trace_time),
               '-o', path, 'sched', 'gfx', 'view', 'wm']
    uiauto_t = threading.Thread(target=tools.start_uiauto, args=(self.name,))
    systrace_t = threading.Thread(target=tools.execute_command, args=(command,))
    uiauto_t.start()
    #time.sleep(self.wait_time)
    systrace_t.start()
    logging.debug('systrace started')
    uiauto_t.join()
    systrace_t.join()
    logging.debug('%s ended', self.name)


CASES = {'facebook_post': TestCase('facebook_post', 10, 15),
         'facebook_comment': TestCase('facebook_comment', 15, 10),
         'facebook_msg': TestCase('facebook_msg', 10, 10),
         'camera': TestCase('camera', 15, 25),
         'camera_shoot': TestCase('camera_shoot', 10, 10),
         'facebook_swipe': TestCase('facebook_swipe', 10, 10),
         'launcher_app': TestCase('launcher_app', 5, 10),
         'youtube': TestCase('youtube', 15, 10),
         'browser': TestCase('browser', 10, 10),
         'launcher': TestCase('launcher', 0, 10),
         'gmail': TestCase('gmail', 8, 15),
         'google_maps': TestCase('google_maps', 15, 15),
         'instagram': TestCase('instagram', 10, 10)}

#
# Copyright (C) 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import subprocess
import sys
from collections import defaultdict
from concurrent import futures
from glob import glob
from operator import itemgetter
import hashlib
tpe = futures.ThreadPoolExecutor()

search_paths = ("system", "product")


def strip_and_sha1sum(filepath):
  def silent_call(cmd):
    return subprocess.call(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) == 0
  objdump = lambda: silent_call(["objdump", "-a", filepath])
  strip_all = lambda: silent_call(["llvm-strip", "--strip-all", "-keep-section=.ARM.attributes", filepath, "-o", filepath + ".tmp"])
  remove_build_id = lambda: silent_call(["llvm-strip", "-remove-section=.note.gnu.build-id", filepath + ".tmp", "-o", filepath + ".tmp.no-build-id"])

  def sha1sum(f):
    with open(f, 'rb') as fin:
      return hashlib.sha1(fin.read()).hexdigest()

  if objdump():
    try:
      if strip_all() and remove_build_id():
        return sha1sum(filepath + ".tmp.no-build-id")
      else:
        return sha1sum(filepath)
    finally:
      if os.path.exists(filepath + ".tmp"):
        os.remove(filepath + ".tmp")
      if os.path.exists(filepath + ".tmp.no-build-id"):
        os.remove(filepath + ".tmp.no-build-id")
  else:
    return sha1sum(filepath)


def main(targets):
  def get_target_name(path):
    return os.path.basename(os.path.normpath(path))
  artifact_target_map = defaultdict(list)
  for target in targets:
    def valid_path(p):
      if os.path.isdir(p) or not os.path.exists(p):
        return False
      for s in search_paths:
        if os.path.join(target, s).lower() + os.path.sep in p.lower():
          return True
      return False
    paths = [path for path in glob(os.path.join(
        target, "**", "*"), recursive=True) if valid_path(path)]

    def run(path):
      return strip_and_sha1sum(path), path[len(target):]
    results = map(futures.Future.result, [tpe.submit(run, p) for p in paths])

    for sha1, f in results:
      basename = os.path.split(os.path.dirname(f))[1] + os.path.basename(f)
      artifact_target_map[(sha1, basename)].append((get_target_name(target), f))

  def pretty_print(p, ts):
    assert(len({t for t, _ in ts}) == len(ts))
    return ";".join({f for _, f in ts}) + ", " + p[0][:10] + ", " + ";".join(map(itemgetter(0), ts)) + "\n"
  header = "filename, sha1sum, targets\n"
  common = sorted([pretty_print(p, ts)
                   for p, ts in artifact_target_map.items() if len(ts) == len(targets)])
  diff = sorted([pretty_print(p, ts)
                 for p, ts in artifact_target_map.items() if len(ts) < len(targets)])

  with open("common.csv", 'w') as fout:
    fout.write(header)
    fout.writelines(common)
  with open("diff.csv", 'w') as fout:
    fout.write(header)
    fout.writelines(diff)


if __name__ == "__main__":
  targets = os.sys.argv[1:]
  if len(targets) < 2:
    print("The number of targets has to be at least two", file=sys.stderr)
    sys.exit(1)
  else:
    main(targets)

#!/usr/bin/env python3
#
# Copyright (C) 2020 The Android Open Source Project
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
"""Fetch a Rust package from crates.io.

Usage: get_rust_pkg.py -v syn-1.0.7
Get the package syn-1.0.7 from crates.io and untar it into ./syn-1.0.7.

Usage: get_rust_pkg.py -v -o tmp syn
Get the latest version of package syn, say 1.0.17,
and untar it into tmp/syn-1.0.17.

This script will abort if the target directory exists.
"""

import argparse
import functools
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

PKG_VERSION_PATTERN = r"(.*)-([0-9]+\.[0-9]+\.[0-9]+.*)"

PKG_VERSION_MATCHER = re.compile(PKG_VERSION_PATTERN)

VERSION_PATTERN = r"([0-9]+)\.([0-9]+)\.([0-9]+)"

VERSION_MATCHER = re.compile(VERSION_PATTERN)


def parse_args():
  """Parse main arguments."""
  parser = argparse.ArgumentParser("get_rust_pkg")
  parser.add_argument(
      "-v", action="store_true", default=False, help="echo executed commands")
  parser.add_argument(
      "-o", metavar="out_dir", default=".", help="output directory")
  parser.add_argument(
      "-tree",
      action="store_true",
      default=False,
      help="find all dependent packages")
  parser.add_argument(
      dest="pkgs",
      metavar="pkg_name",
      nargs="+",
      help="name of Rust package to be fetched from crates.io")
  return parser.parse_args()


def set2list(a_set):
  if a_set:
    return " " + " ".join(sorted(a_set))
  else:
    return ""


def echo(args, msg):
  if args.v:
    print("INFO: {}".format(msg))


def echo_all_deps(args, kind, deps):
  if args.v and deps:
    print("INFO: now {} in {}:{}".format(len(deps), kind, set2list(deps)))


def pkg_base_name(pkg):
  match = PKG_VERSION_MATCHER.match(pkg)
  if match is not None:
    return (match.group(1), match.group(2))
  else:
    return (pkg, "")


def find_pkg_base_name(args, name):
  """Remove version string of name."""
  (base, version) = pkg_base_name(name)
  if version:
    echo(args, "package base name: {}  version: {}".format(base, version))
  else:
    echo(args, "package base name: {}".format(base))
  return base, version


def get_version_numbers(version):
  match = VERSION_MATCHER.match(version)
  if match is not None:
    return tuple(int(match.group(i)) for i in range(1, 4))
  return (0, 0, 0)


def is_newer_version(args, prev_version, prev_id, check_version, check_id):
  """Return true if check_version+id is newer than prev_version+id."""
  echo(args, "checking version={}  id={}".format(check_version, check_id))
  return ((get_version_numbers(check_version), check_id) >
          (get_version_numbers(prev_version), prev_id))


def find_dl_path(args, name):
  """Ask crates.io for the latest version download path."""
  base_name, version = find_pkg_base_name(args, name)
  url = "https://crates.io/api/v1/crates/{}/versions".format(base_name)
  echo(args, "get versions at {}".format(url))
  with urllib.request.urlopen(url) as request:
    data = json.loads(request.read().decode())
    # version with the largest id number is assumed to be the latest
    last_id = 0
    dl_path = ""
    found_version = ""
    for v in data["versions"]:
      # Return the given version if it is found.
      if version == v["num"]:
        dl_path = v["dl_path"]
        found_version = version
        break
      if version:  # must find user specified version
        continue
      # Skip yanked version.
      if v["yanked"]:
        echo(args, "skip yanked version {}".format(v["num"]))
        continue
      # Remember the newest version.
      if is_newer_version(args, found_version, last_id, v["num"], int(v["id"])):
        last_id = int(v["id"])
        found_version = v["num"]
        dl_path = v["dl_path"]
    if not dl_path:
      print("ERROR: cannot find version {} of package {}".format(
          version, base_name))
      return None
    echo(args, "found download path for version {}".format(found_version))
    return dl_path


def fetch_pkg(args, dl_path):
  """Fetch package from crates.io and untar it into a subdirectory."""
  if not dl_path:
    return False
  url = "https://crates.io" + dl_path
  tmp_dir = tempfile.mkdtemp()
  echo(args, "fetch tar file from {}".format(url))
  tar_file, _ = urllib.request.urlretrieve(url)
  with tarfile.open(tar_file, mode="r") as tfile:
    echo(args, "extract tar file {} into {}".format(tar_file, tmp_dir))
    tfile.extractall(tmp_dir)
    files = os.listdir(tmp_dir)
    # There should be only one directory in the tar file,
    # but it might not be (name + "-" + version)
    pkg_tmp_dir = os.path.join(tmp_dir, files[0])
    echo(args, "untared package in {}".format(pkg_tmp_dir))
    dest_dir = os.path.join(args.o, files[0])
    if os.path.exists(dest_dir):
      print("ERROR: do not overwrite existing {}".format(dest_dir))
      return None  # leave tar_file and tmp_dir
    else:
      echo(args, "move {} to {}".format(pkg_tmp_dir, dest_dir))
      shutil.move(pkg_tmp_dir, dest_dir)
  echo(args, "delete downloaded tar file {}".format(tar_file))
  os.remove(tar_file)
  echo(args, "delete temp directory {}".format(tmp_dir))
  shutil.rmtree(tmp_dir)
  print("SUCCESS: downloaded package in {}".format(dest_dir))
  return dest_dir


def call_cargo_tree_cmd(roots, dest_dir, cmd, kind):
  """Call cargo tree with given cmd and return dependent pkgs."""
  proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, cwd=dest_dir)
  out = proc.stdout.read()
  if proc.wait():
    print("ERROR: in {}, {}".format(dest_dir, " ".join(cmd)))
    return set()
  lines = out.decode("utf-8").split("\n")
  # The 1st line is the current package; the last is empty.
  root = re.sub(r" \(.*\)", "", lines[0])
  pkgs = set()
  for line in lines[1:-1]:
    pkgs.add(line.split(" ")[0])
  pkgs = pkgs.difference(roots)
  print("NOTE: {} has {} {} package(s):{}".format(root, len(pkgs), kind,
                                                  set2list(pkgs)))
  return pkgs


def call_cargo_tree(roots, dest_dir):
  """Call cargo tree in dest_dir and return dependent pkgs."""
  cmd1 = ["cargo", "tree", "--no-indent", "--quiet", "--no-dev-dependencies"]
  cmd2 = ["cargo", "tree", "--no-indent", "--quiet"]
  return (call_cargo_tree_cmd(roots, dest_dir, cmd1, "non-dev-dependent"),
          call_cargo_tree_cmd(roots, dest_dir, cmd2, "dependent"))


def find_cargo_tree():
  try:
    proc = subprocess.Popen(["cargo", "tree", "-h"], stdout=subprocess.PIPE)
    if proc.wait():
      print("ERROR: error in cargo tree -h")
      return False
  except FileNotFoundError:
    print("ERROR: cannot find cargo tree")
    return False
  return True


def compare_pkg_deps(pkg1, pkg2):
  """Compare dependency order of pkg1 and pkg2."""
  base1, _, deps1, dev_deps1, _, _ = pkg1
  base2, _, deps2, dev_deps2, _, _ = pkg2
  if base1 in deps2:
    return -1  # pkg2 needs base1
  if base2 in deps1:
    return 1  # pkg1 needs base2
  if base1 in dev_deps2:
    return -1  # pkg2 needs base1
  if base2 in dev_deps1:
    return 1  # pkg1 needs base2
  count1 = (len(deps1), len(dev_deps1), base1)
  count2 = (len(deps2), len(dev_deps2), base2)
  if count1 != count2:
    # smaller dependent list or shorter base name goes first
    return -1 if count1 < count2 else 1
  return 0


def sort_found_deps(tuples):
  """A topological sort of tuples based on all_deps."""
  # tuples is list of (base_name, pkg, deps, dev_deps, all_deps, all_dev_deps)
  # collect external base names; remove them from all*deps
  all_base_names = set(map(lambda t: t[0], tuples))
  all_names = set()
  for (_, _, _, _, all_deps, all_dev_deps) in tuples:
    all_names = all_names.union(all_deps).union(all_dev_deps)
  other_names = all_names.difference(all_base_names)

  def remove_other_names(t):
    return (t[0], t[1], t[2], t[3], t[4].difference(other_names),
            t[5].difference(other_names))

  new_tuples = list(map(remove_other_names, tuples))
  # sorted_tuples is list of (base_name, pkg, deps, dev_deps)
  sorted_tuples = []
  while new_tuples:
    # tuples with empty all_deps are put before the others
    first_group = list(filter(lambda t: not t[4], new_tuples))
    other_group = list(filter(lambda t: t[4], new_tuples))
    new_tuples = []
    if first_group:
      # tuples in first_group are order by compare_pkg_deps
      first_group.sort(key=functools.cmp_to_key(compare_pkg_deps))
      sorted_tuples.extend(list(map(lambda t: t[:4], first_group)))
      # copy other_group to new_tuples but filter names in the first_group
      base_names = set(map(lambda t: t[0], first_group))

      def remove_base_names(t):
        return (t[0], t[1], t[2], t[3], t[4].difference(base_names),
                t[5].difference(base_names))

      new_tuples = list(map(remove_base_names, other_group))
    else:
      print("ERROR: leftover in topological sort: {}".format(
          list(map(lambda t: t[1], other_group))))
      other_group.sort(key=functools.cmp_to_key(compare_pkg_deps))
      sorted_tuples.extend(list(map(lambda t: t[:4], other_group)))
  return sorted_tuples


def check_dependencies(args, packages, dirs):
  """Calls cargo tree to check dependent packages."""
  if not dirs or not packages:
    return
  found_deps = []
  all_pkgs = set()
  for dest_dir in dirs:
    pkg = os.path.basename(dest_dir)
    if pkg in all_pkgs:
      print("ERROR: duplicated {}", pkg)
      continue
    all_pkgs.add(pkg)
    echo(args, "checking dependent packages of {}".format(pkg))
    base_name, _ = pkg_base_name(pkg)
    deps, dev_deps = call_cargo_tree(set([base_name]), dest_dir)
    found_deps.append((base_name, pkg, deps, dev_deps, deps, dev_deps))
  if not found_deps:
    return

  found_deps = sort_found_deps(found_deps)
  max_pkg_length = 0
  for (_, pkg, _, _) in found_deps:
    max_pkg_length = max(max_pkg_length, len(pkg))
  name_format = "{:" + str(max_pkg_length) + "}"
  print("\n##### Summary of all dependent package counts #####")
  print("deps[k] = # of non-dev-dependent packages of pkg[k]")
  print("dev_deps[k] = # of all dependent packages of pkg[k]")
  print("all_deps[k] = # of non-dev-dependent packages of pkg[1] to pkg[k]")
  print("all_dev_deps[k] = # of all dependent packages of pkg[1] to pkg[k]")
  print(("{:>4s} " + name_format + " {:>8s} {:>8s} {:>8s} {:>8s}").format(
      "k", "pkg", "deps", "dev_deps", "all_deps", "all_dev_deps"))
  roots = set()
  all_pkgs = set()
  all_deps = set()
  all_dev_deps = set()
  k = 0
  for (base_name, pkg, deps, dev_deps) in found_deps:
    roots.add(base_name)
    all_pkgs.add(pkg)
    all_deps = all_deps.union(deps).difference(roots)
    all_dev_deps = all_dev_deps.union(dev_deps).difference(roots)
    k += 1
    print(("{:4d} " + name_format + " {:8d} {:8d} {:8d} {:8d}").format(
        k, pkg, len(deps), len(dev_deps), len(all_deps), len(all_dev_deps)))
    echo_all_deps(args, "all_deps", all_deps)
    echo_all_deps(args, "all_dev_deps", all_dev_deps)
  print("\nNOTE: from all {} package(s):{}".format(
      len(all_pkgs), set2list(all_pkgs)))
  print("NOTE: found {:3d} other non-dev-dependent package(s):{}".format(
      len(all_deps), set2list(all_deps)))
  print("NOTE: found {:3d} other dependent package(s):{}".format(
      len(all_dev_deps), set2list(all_dev_deps)))


def main():
  args = parse_args()
  if args.tree and not find_cargo_tree():
    sys.exit(1)
  echo(args, "to fetch packages = {}".format(args.pkgs))
  errors = []
  dirs = []
  checked_pkgs = set()
  found_packages = []
  for pkg in args.pkgs:
    if pkg in checked_pkgs:
      print("WARNIG: skip duplicated pkg: {}".format(pkg))
      continue
    checked_pkgs.add(pkg)
    echo(args, "trying to fetch package {}".format(pkg))
    try:
      dest_dir = fetch_pkg(args, find_dl_path(args, pkg))
      if dest_dir:
        if dest_dir not in dirs:
          dirs.append(dest_dir)
          found_packages.append(pkg)
      else:
        errors.append(pkg)
    except urllib.error.HTTPError:
      errors.append(pkg)
  if errors:
    for pkg in errors:
      print("ERROR: failed to fetch {}".format(pkg))
  if args.tree and dirs:
    check_dependencies(args, found_packages, dirs)
  if errors:
    sys.exit(1)


if __name__ == "__main__":
  main()

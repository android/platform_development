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
  # Cargo tree default has dev-dependent packages, we call it dev_deps.
  # With --no-dev-dependencies, we call the output build_deps.
  return (call_cargo_tree_cmd(roots, dest_dir, cmd1, "build-dependent"),
          call_cargo_tree_cmd(roots, dest_dir, cmd2, "dev-dependent"))


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
  base1, pkg_name1, build_deps1, dev_deps1 = pkg1
  base2, pkg_name2, build_deps2, dev_deps2 = pkg2
  # Some pkg1 can be build-dependent (non-dev-dependent) on pkg2,
  # when pkg2 is only test-dependent (dev-dependent) on pkg1.
  # This is not really a build dependency cycle, because pkg2
  # can be built before pkg1, but tested after pkg1.
  # So the dependency order is based on build_deps first, and then dev_deps.
  if base1 in build_deps2:
    return -1  # pkg2 needs base1
  if base2 in build_deps1:
    return 1  # pkg1 needs base2
  if base1 in dev_deps2:
    return -1  # pkg2 needs base1
  if base2 in dev_deps1:
    return 1  # pkg1 needs base2
  # If there is no dependency between pkg1 and pkg2,
  # order them by the size of build_deps or dev_deps, or the name.
  count1 = (len(build_deps1), len(dev_deps1), base1, pkg_name1)
  count2 = (len(build_deps2), len(dev_deps2), base2, pkg_name2)
  if count1 != count2:
    return -1 if count1 < count2 else 1
  return 0


def sort_found_pkgs(tuples):
  """A topological sort of tuples based on build_deps."""
  # tuples is a list of (base_name, pkg, build_deps, dev_deps)

  # Use build_deps as the dependency relation in a topological sort.
  # The new_tuples list is used in topological sort. It is the input tuples
  # prefixed with a changing build_deps during the sorting process.
  # Collect all package base names.
  # Dependent packages not found in all_base_names will be treated as
  # "external" and ignored in topological sort.
  all_base_names = set(map(lambda t: t[0], tuples))
  new_tuples = []
  all_names = set()
  for (base_name, pkg, build_deps, dev_deps) in tuples:
    new_tuples.append((build_deps, (base_name, pkg, build_deps, dev_deps)))
    all_names = all_names.union(build_deps)
  external_names = all_names.difference(all_base_names)
  new_tuples = list(
      map(lambda t: (t[0].difference(external_names), t[1]), new_tuples))

  sorted_tuples = []
  # A brute force topological sort;
  # tuples with empty build_deps are put before the others.
  while new_tuples:
    first_group = list(filter(lambda t: not t[0], new_tuples))
    other_group = list(filter(lambda t: t[0], new_tuples))
    new_tuples = []
    if first_group:
      # Remove the extra build_deps in first_group,
      # then sort it, and add its tuples to the sorted_tuples list.
      first_group = list(map(lambda t: t[1], first_group))
      first_group.sort(key=functools.cmp_to_key(compare_pkg_deps))
      sorted_tuples.extend(first_group)
      # Copy other_group to new_tuples but remove names in the first_group.
      base_names = set(map(lambda t: t[0], first_group))
      new_tuples = list(
          map(lambda t: (t[0].difference(base_names), t[1]), other_group))
    else:
      print("ERROR: leftover in topological sort: {}".format(
          list(map(lambda t: t[1][1], other_group))))
      # Anyway, sort the other_group to include them into final report.
      other_group = list(map(lambda t: t[1], other_group))
      other_group.sort(key=functools.cmp_to_key(compare_pkg_deps))
      sorted_tuples.extend(other_group)
  return sorted_tuples


def check_dependencies(args, packages, dirs):
  """Calls cargo tree to count dependent packages; returns True on success."""
  if not dirs or not packages:
    return False

  success = True
  found_pkgs = []
  all_pkgs = set()
  for dest_dir in dirs:
    pkg = os.path.basename(dest_dir)
    if pkg in all_pkgs:
      print("ERROR: duplicated {}", pkg)
      success = False
      continue  # to produce as many dependency reports as possible
    all_pkgs.add(pkg)
    echo(args, "checking dependent packages of {}".format(pkg))
    base_name, _ = pkg_base_name(pkg)
    build_deps, dev_deps = call_cargo_tree(set([base_name]), dest_dir)
    found_pkgs.append((base_name, pkg, build_deps, dev_deps))
  if not found_pkgs:
    return False

  found_pkgs = sort_found_pkgs(found_pkgs)
  max_pkg_length = 0
  for (_, pkg, _, _) in found_pkgs:
    max_pkg_length = max(max_pkg_length, len(pkg))
  name_format = "{:" + str(max_pkg_length) + "s}"
  print("\n##### Summary of all dependent package counts #####")
  print("build_deps[k] = # of non-dev-dependent packages of pkg[k]")
  print("dev_deps[k] = # of all dependent packages of pkg[k]")
  print(
      "all_build_deps[k] = # of non-dev-dependent packages of pkg[1] to pkg[k]")
  print("all_dev_deps[k] = # of all dependent packages of pkg[1] to pkg[k]")
  print(("{:>4s} " + name_format + " {:>10s} {:>10s} {:>14s} {:>14s}").format(
      "k", "pkg", "build_deps", "dev_deps", "all_build_deps", "all_dev_deps"))
  roots = set()
  all_pkgs = set()
  all_build_deps = set()
  all_dev_deps = set()
  k = 0
  for (base_name, pkg, build_deps, dev_deps) in found_pkgs:
    roots.add(base_name)
    all_pkgs.add(pkg)
    all_build_deps = all_build_deps.union(build_deps).difference(roots)
    all_dev_deps = all_dev_deps.union(dev_deps).difference(roots)
    k += 1
    print(("{:4d} " + name_format + " {:10d} {:10d} {:14d} {:14d}").format(
        k, pkg, len(build_deps), len(dev_deps), len(all_build_deps),
        len(all_dev_deps)))
    echo_all_deps(args, "all_build_deps", all_build_deps)
    echo_all_deps(args, "all_dev_deps", all_dev_deps)
  print("\nNOTE: from all {} package(s):{}".format(
      len(all_pkgs), set2list(all_pkgs)))
  print("NOTE: found {:3d} other non-dev-dependent package(s):{}".format(
      len(all_build_deps), set2list(all_build_deps)))
  print("NOTE: found {:3d} other dependent package(s):{}".format(
      len(all_dev_deps), set2list(all_dev_deps)))
  return success


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
      print("WARNING: skip duplicated pkg: {}".format(pkg))
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
    if not check_dependencies(args, found_packages, dirs):
      sys.exit(2)
  if errors:
    sys.exit(1)


if __name__ == "__main__":
  main()

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

cargo_not_found = False


def parse_args():
  """Parse main arguments."""
  parser = argparse.ArgumentParser("get_rust_pkg")
  parser.add_argument(
      "-v", action="store_true", default=False,
      help="echo executed commands")
  parser.add_argument(
      "-o", metavar="out_dir", default=".",
      help="output directory")
  parser.add_argument(
      "-tree", action="store_true", default=False,
      help="find all dependent packages")
  parser.add_argument(
      dest="pkgs", metavar="pkg_name", nargs="+",
      help="name of Rust package to be fetched from crates.io")
  return parser.parse_args()


def echo(args, msg):
  if args.v:
    print("INFO: {}".format(msg))


def pkg_base_name(args, name):
  """Remove version string of name."""
  base = name
  version = ""
  match = PKG_VERSION_MATCHER.match(name)
  if match is not None:
    base = match.group(1)
    version = match.group(2)
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
  base_name, version = pkg_base_name(args, name)
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
      print("ERROR: cannot find version {} of package {}"
            .format(version, base_name))
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


def call_cargo_tree_cmd(dest_dir, roots, cmd, kind, deps):
  """Call cargo tree with given cmd and add dependent pkgs into deps."""
  proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, cwd=dest_dir)
  out = proc.stdout.read()
  if proc.wait():
    print("ERROR: {}".format(" ".join(cmd)))
    return False
  lines = out.decode("utf-8").split("\n")
  # The 1st line is the current package; the last is empty.
  root = re.sub(r" \(.*\)", "", lines[0])
  others = set()
  for line in lines[1:-1]:
    pkg = line.split(" ")[0]
    if pkg not in roots:
      others.add(pkg)
      deps.add(pkg)
  others = sorted(others)
  print("NOTE: {} {}-dependent packages of {}: {}".
        format(len(others), kind, root, others))
  return True


def call_cargo_tree(dest_dir, roots, deps, dev_deps):
  """Call cargo tree to find and fetch dependent packages of pkg in dest_dir."""
  global cargo_not_found
  if cargo_not_found:
    return False
  try:
    cmd1 = ["cargo", "tree", "--no-indent", "--quiet", "--no-dev-dependencies"]
    # If cargo tree works with "--no-dev-dependencies", try without that flag.
    if call_cargo_tree_cmd(dest_dir, roots, cmd1, "without-dev", deps):
      cmd2 = ["cargo", "tree", "--no-indent", "--quiet"]
      if call_cargo_tree_cmd(dest_dir, roots, cmd2, "with-dev", dev_deps):
        return True
  except FileNotFoundError:
    cargo_not_found = True
    print("ERROR: cannot find cargo tree")
  return False


def check_dependencies(args, packages, dirs):
  """Calls cargo tree to check dependent packages."""
  roots = set()
  for pkg in packages:
    match = PKG_VERSION_MATCHER.match(pkg)
    if match is not None:
      roots.add(match.group(1))
    else:
      roots.add(pkg)
  deps = set()  # dependent pkgs excluding dev-dependencies
  dev_deps = set()  # dependent pkgs including dev-dependencies
  success = True
  for dest_dir in dirs:
    pkg = os.path.basename(dest_dir)
    echo(args, "checking dependent packages of {}".format(pkg))
    if not call_cargo_tree(dest_dir, roots, deps, dev_deps):
      print("ERROR: failed to check dependent packages of {}".format(pkg))
      success = False
  print("NOTE: for all {} root package(s): {}".
        format(len(roots), sorted(roots)))
  print("NOTE: found {} non-dev-dependent package(s): {}".
        format(len(deps), sorted(deps)))
  print("NOTE: found {} dependent package(s): {}".
        format(len(dev_deps), sorted(dev_deps)))
  return success


def main():
  args = parse_args()
  packages = list(dict.fromkeys(args.pkgs))
  echo(args, "to fetch packags = {}".format(packages))
  errors = []
  dirs = []
  found_packages = []
  for pkg in packages:
    echo(args, "trying to fetch package {}".format(pkg))
    try:
      dest_dir = fetch_pkg(args, find_dl_path(args, pkg))
      if dest_dir:
        dirs.append(dest_dir)
        found_packages.append(pkg)
      else:
        errors.append(pkg)
    except urllib.error.HTTPError:
      errors.append(pkg)
  success = True
  if args.tree and dirs:
    success = check_dependencies(args, found_packages, dirs)
  if errors:
    for pkg in errors:
      print("ERROR: failed to fetch {}".format(pkg))
    sys.exit(1)
  if not success:
    sys.exit(2)


if __name__ == "__main__":
  main()

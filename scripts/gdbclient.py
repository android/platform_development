#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
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
#

import adb
import argparse
import logging
import os
import sys
import tempfile

# Shared functions across gdbclient.py and ndk-gdb.py.
from gdbrunner import *

def check_product_out_dir(device, product):
    root = os.environ["ANDROID_BUILD_TOP"]
    path = "{}/out/target/product/{}".format(root, product)
    if os.path.isdir(path):
        return path
    return None

def get_gdbserver_path(root, arch):
    path = "{}/prebuilts/misc/android-{}/gdbserver{}/gdbserver{}"
    if arch.endswith("64"):
        return path.format(root, arch, "64", "64")
    else:
        return path.format(root, arch, "", "")

def main():
    parser = adb.ArgumentParser()

    group = parser.add_argument_group(title="attach target")
    group = group.add_mutually_exclusive_group(required=True)
    group.add_argument(
        "-p", dest="target_pid", metavar="PID",
        help="attach to a process with specified PID")
    group.add_argument(
        "-n", dest="target_name", metavar="NAME",
        help="attach to a process with specified name")
    group.add_argument(
        "-r", dest="run_cmd", metavar="CMD", nargs="+",
        help="run a binary on the device, with args")
    group.add_argument(
        "-u", dest="upload_cmd", metavar="CMD", nargs="+",
        help="upload and run a binary on the device, with args")

    parser.add_argument(
        "--port", nargs="?", default="5039",
        help="override the port used on the host")
    parser.add_argument(
        "--user", nargs="?", default="root",
        help="user to run commands as on the device [default: root]")

    args = parser.parse_args()
    device = args.device
    if "ANDROID_BUILD_TOP" not in os.environ:
        raise RuntimeError("$ANDROID_BUILD_TOP is not set. " +
                           "Source build/envsetup.sh and run lunch.")

    root = os.environ["ANDROID_BUILD_TOP"]
    if not os.path.exists(root):
        raise RuntimeError("ANDROID_BUILD_TOP [{}] doesn't exist".format(root))
    if not os.path.isdir(root):
        raise RuntimeError("ANDROID_BUILD_TOP [{}] isn't a directory".format(root))


    props = device.get_props()
    if "ro.hardware" in props:
        out_dir = check_product_out_dir(device, props["ro.hardware"])

    if out_dir is None and "ro.product.device" in props:
        out_dir = check_product_out_dir(device, props["ro.product.device"])

    if out_dir is None:
        msg = ("Failed to find product out directory for ro.hardware = '{}', " +
               "ro.product.device = '{}'")
        msg = msg.format(props.get("ro.hardware", ""),
                         props.get("ro.product.device", ""))
        raise RuntimeError(msg)

    sysroot = os.path.join(out_dir, "symbols")

    if args.target_pid and not unicode(args.target_pid).isdecimal():
        raise RuntimeError("invalid pid '{}'".format(args.target_pid))

    props = device.get_props()

    debug_socket = "/data/local/tmp/debug_socket"
    pid = None
    process_name = None
    binary = None
    run_cmd = None

    if args.target_pid:
        # Fetch the binary using the PID later.
        pid = int(args.target_pid)
        processes = get_processes(device)

        for k, v in processes.iteritems():
            if pid in v:
                process_name = k
        if process_name is None:
            msg = "failed to find process id {}".format(pid)
            raise RuntimeError(msg)
    elif args.target_name:
        process_name = args.target_name
        processes = get_processes(device)
        if args.target_name not in processes:
            msg = "failed to find running process {}".format(processname)
            raise RuntimeError(msg)
        pids = processes[args.target_name]
        if len(pids) > 1:
            msg = "multiple processes match '{}': {}".format(process_name, pids)
            raise RuntimeError(msg)

        # Fetch the binary using the PID later.
        pid = pids[0]
    elif args.run_cmd:
        if not args.run_cmd[0]:
            raise RuntimeError("empty command passed to -r")
        if not args.run_cmd[0].startswith("/"):
            raise RuntimeError("commands passed to -r must use absolute paths")
        binary = pull_file(device, args.run_cmd[0], user=args.user)
        run_cmd = args.run_cmd
    elif args.upload_cmd:
        if not args.upload_cmd[0]:
            raise RuntimeError("empty command passed to -u")
        if not args.upload_cmd[0].startswith("/"):
            raise RuntimeError("commands passed to -u must use absolute paths")
        run_cmd = args.upload_cmd
        remote_path = args.upload_cmd[0]
        local_path = os.path.normpath("{}/{}".format(sysroot, remote_path))
        if not os.path.exists(local_path):
            raise RuntimeError("nonexistent upload path {}".format(local_path))
        if not os.path.isfile(local_path):
            raise RuntimeError("upload path {} isn't a file".format(local_path))

        logging.info("uploading {} to {}".format(local_path, remote_path))
        logging.warning("running adb root")
        device.root()
        remount_msg = device.remount()
        if "verity" in remount_msg:
            print remount_msg
            sys.exit(1)

        with open(local_path, "r") as binary_file:
            binary = binary_file.read()

        device.push(local_path, remote_path)

    # Fetch binary for -p, -n.
    if not binary:
        try:
            binary = pull_binary(device, pid=pid, user=args.user)
        except adb.ShellError:
            raise RuntimeError("failed to pull binary for PID {}".format(pid))

    arch = get_binary_arch(binary)
    is64bit = arch.endswith("64")

    # Start gdbserver.
    gdbserver_local_path = get_gdbserver_path(root, arch)
    gdbserver_remote_path = "/data/local/tmp/{}-gdbserver".format(arch)
    start_gdbserver(
        device, gdbserver_local_path, gdbserver_remote_path,
        target_pid=pid, run_cmd=run_cmd, debug_socket=debug_socket,
        port=args.port, user=args.user)

    # Generate a gdb script.
    # TODO: Make stuff work for tapas?
    # TODO: Detect the zygote and run 'art-on' automatically.
    binary_file = tempfile.NamedTemporaryFile()
    binary_file.write(binary)
    binary_file.flush()

    symbols_dir = os.path.join(sysroot, "system", "lib64" if is64bit else "lib")
    vendor_dir = os.path.join(sysroot, "vendor", "lib64" if is64bit else "lib")

    solib_search_path = []
    symbols_paths = ["", "hw", "ssl/engines", "drm", "egl", "soundfx"]
    vendor_paths = ["", "hw", "egl"]
    solib_search_path += [os.path.join(symbols_dir, x) for x in symbols_paths]
    solib_search_path += [os.path.join(vendor_dir, x) for x in vendor_paths]
    solib_search_path = ":".join(solib_search_path)

    gdb_commands = ""
    gdb_commands += "file '{}'\n".format(binary_file.name)
    gdb_commands += "set solib-absolute-prefix {}\n".format(sysroot)
    gdb_commands += "set solib-search-path {}\n".format(solib_search_path)

    dalvik_gdb_script = os.path.join(root, "development", "scripts", "gdb",
                                     "dalvik.gdb")
    if not os.path.exists(dalvik_gdb_script):
        logging.warning(("couldn't find {} - ART debugging options will not " +
                         "be available").format(dalvik_gdb_script))
    else:
        gdb_commands += "source {}\n".format(dalvik_gdb_script)

    gdb_commands += "target remote :{}\n".format(args.port)
    gdb_path = os.path.join(root, "prebuilts", "gdb", "linux-x86", "bin", "gdb")
    start_gdb(gdb_path, gdb_commands)

if __name__ == "__main__":
    main()

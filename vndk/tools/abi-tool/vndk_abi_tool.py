#!/usr/bin/env python3

from __future__ import print_function

import argparse
import os
import subprocess
import sys
import traceback

if sys.version_info >= (3, 0):
    from os import makedirs
    from shutil import which

    def get_byte(buf, idx):
        return buf[idx]

    def check_silent_call(cmd):
        subprocess.check_call(cmd, stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL)
else:
    def makedirs(path, exist_ok):
        if exist_ok and os.path.isdir(path):
            return
        return os.makedirs(path)

    def which(cmd, mode=os.F_OK | os.X_OK, path=None):
        def is_executable(path):
            return (os.path.exists(file_path) and \
                    os.access(file_path, mode) and \
                    not os.path.isdir(file_path))
        if path is None:
            path = os.environ.get('PATH', os.defpath)
        for path_dir in path.split(os.pathsep):
            for file_name in os.listdir(path_dir):
                if file_name != cmd:
                    continue
                file_path = os.path.join(path_dir, file_name)
                if is_executable(file_path):
                    return file_path
        return None

    def get_byte(buf, idx):
        return ord(buf[idx])

    def check_silent_call(cmd):
        with open(os.devnull, 'wb') as devnull:
            subprocess.check_call(cmd, stdout=devnull, stderr=devnull)

    FileNotFoundError = OSError

# Path constants.
SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
AOSP_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, *['..'] * 4))
ABI_DUMPER = os.path.join(AOSP_DIR, 'external', 'abi-dumper', 'abi-dumper.pl')
VTABLE_DUMPER = 'vndk-vtable-dumper'
READELF = 'readelf'
OBJDUMP = 'objdump'

def test_command(name, options, expected_output):
    def is_command_valid():
        try:
            if os.path.exists(name) and os.access(name, os.F_OK | os.X_OK):
                exec_path = name
            else:
                exec_path = which(name)
                if not exec_path:
                    return False
            output = subprocess.check_output([exec_path] + options)
            return (expected_output in output)
        except Exception:
            traceback.print_exc()
            return False

    if not is_command_valid():
        print('error: failed to run {} command'.format(name), file=sys.stderr)
        sys.exit(1)

def test_readelf_command():
    test_command(READELF, ['-v'], b'GNU readelf')

def test_objdump_command():
    test_command(OBJDUMP, ['-v'], b'GNU objdump')

def test_vtable_dumper_command():
    test_command(VTABLE_DUMPER, ['--version'], b'vndk-vtable-dumper')

def test_abi_dumper_command():
    test_command(ABI_DUMPER, ['-v'], b'ABI Dumper')

def test_all_commands():
    """Test all external commands used by abi-dumper"""
    test_readelf_command()
    test_objdump_command()
    test_vtable_dumper_command()
    test_abi_dumper_command()

# ELF file format constants.
EI_CLASS = 4
EI_DATA = 5
EI_NIDENT = 8

ELFCLASS32 = 1
ELFCLASS64 = 2

ELFDATA2LSB = 1
ELFDATA2MSB = 2

def is_elf_ident(buf):
    # Check the length of ELF ident.
    if len(buf) != EI_NIDENT:
        return False
    # Check ELF magic word.
    if buf[0:4] != b'\x7fELF':
        return False
    # Check ELF machine word size.
    ei_class = get_byte(buf, EI_CLASS)
    if ei_class != ELFCLASS32 and ei_class != ELFCLASS64:
        return False
    # Check ELF endianness.
    ei_data = get_byte(buf, EI_DATA)
    if ei_data != ELFDATA2LSB and ei_data != ELFDATA2MSB:
        return False
    return True

def is_elf_file(path):
    try:
        with open(path, 'rb') as f:
            return is_elf_ident(f.read(EI_NIDENT))
    except FileNotFoundError:
        return False

def create_abi_reference_dump(out_dir, symbols_system_dir, api_level):
    cmd_base = [ABI_DUMPER, '-lver', api_level, '-objdump', which(OBJDUMP),
                '-readelf', which(READELF), '-vt-dumper', which(VTABLE_DUMPER),
                '-use-tu-dump', '--quiet']

    num_processed = 0
    symbols_system_dir = os.path.abspath(symbols_system_dir)
    prefix_len = len(symbols_system_dir) + 1
    for base, dirnames, filenames in os.walk(symbols_system_dir):
        for filename in filenames:
            if not filename.endswith('.so'):
                continue

            path = os.path.join(base, filename)
            if not is_elf_file(path):
                continue

            rel_path = path[prefix_len:]
            out_path = os.path.join(out_dir, rel_path) + '.dump'

            makedirs(os.path.dirname(out_path), exist_ok=True)
            cmd = cmd_base + [path, '-o', out_path]
            print('running:', ' '.join(cmd))
            check_silent_call(cmd)
            num_processed += 1

    print('done (processed: {})'.format(num_processed))

def main():
    # Parse command line options.
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', '-o', metavar='path', required=True,
                        help='output directory for abi reference dump')
    parser.add_argument('--api-level', default='24', help='VNDK API level')
    parser.add_argument('dir', help='unstripped system image directory')
    args = parser.parse_args()

    # Test commands.
    test_all_commands()

    # Generate ABI dump for all ELF files.
    create_abi_reference_dump(args.output, args.dir, args.api_level)

if __name__ == '__main__':
    main()

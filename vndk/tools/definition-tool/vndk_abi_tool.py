#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys

if sys.version_info >= (3, 0):
    from os import makedirs

    def check_silent_call(cmd):
        subprocess.check_call(cmd, stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL)
else:
    def makedirs(path, exist_ok):
        if exist_ok and os.path.isdir(path):
            return
        return os.makedirs(path)

    def check_silent_call(cmd):
        with open(os.devnull, 'wb') as devnull:
            subprocess.check_call(cmd, stdout=devnull, stderr=devnull)

    FileNotFoundError = OSError

# Path constants.
SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
AOSP_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, *['..'] * 4))
#ABI_DUMPER = os.path.join(AOSP_DIR, 'external', 'abi-dumper', 'abi-dumper.pl')
ABI_DUMPER = 'abi-dumper'

# External command testing function.
def test_readelf_command():
    output = subprocess.check_output(['readelf', '-v'])
    if b'GNU readelf' not in output:
        print('error: unsupported readelf command', file=sys.stderr)
        sys.exit(1)

def test_abi_dumper_command():
    output = subprocess.check_output([ABI_DUMPER, '-v'])
    if b'ABI Dumper' not in output:
        print('error: failed to run abi-dumper:', ABI_DUMPER, file=sys.stderr)
        sys.exit(1)

def test_command():
    test_readelf_command()
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
    ei_class = buf[EI_CLASS]
    if ei_class != ELFCLASS32 and ei_class != ELFCLASS64:
        return False
    # Check ELF endianness.
    ei_data = buf[EI_DATA]
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

            print('processing', rel_path, '...')
            os.makedirs(os.path.dirname(out_path), exist_ok=True)
            cmd = [ABI_DUMPER, path, '-o', out_path, '-lver', api_level]
            check_silent_call(cmd)

def main():
    # Parse command line options.
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', '-o', metavar='path', required=True,
                        help='output directory for abi reference dump')
    parser.add_argument('--api-level', default='24', help='VNDK API level')
    parser.add_argument('dir', help='unstripped system image directory')
    args = parser.parse_args()

    # Test commands.
    test_command()

    # Generate ABI dump for all ELF files.
    create_abi_reference_dump(args.output, args.dir, args.api_level)

if __name__ == '__main__':
    main()

#!/usr/bin/env python3

import tempfile
import os
import subprocess

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
AOSP_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, *['..'] * 5))

BUILTIN_HEADERS_DIR = (
    os.path.join(AOSP_DIR, 'bionic', 'libc', 'include'),
    os.path.join(AOSP_DIR, 'external', 'libcxx', 'include'),
    os.path.join(AOSP_DIR, 'prebuilts', 'sdk', 'renderscript', 'clang-include'),
)

EXPORTED_HEADERS_DIR = (
    os.path.join(AOSP_DIR, 'development', 'vndk', 'tools', 'header-checker',
                 'tests'),
)

SOURCE_ABI_DUMP_EXT = ".so.lsdump"

def read_output_content(output_path, replace_str):
    with open(output_path, 'r') as f:
        return f.read().replace(replace_str, '.')

def run_header_checker(input_path, cflags=[]):
    with tempfile.TemporaryDirectory() as tmp:
        output_name = os.path.join(tmp, os.path.basename(input_path)) + '.dump'
        cmd = ['header-abi-dumper', '-o', output_name, input_path,]
        for d in EXPORTED_HEADERS_DIR:
            cmd += ['-I', d]
        cmd+= ['--']
        for d in BUILTIN_HEADERS_DIR:
            cmd += ['-isystem', d]
        cmd += cflags
        subprocess.check_call(cmd)
        with open(output_name, 'r') as f:
          return read_output_content(output_name, SCRIPT_DIR)

def make_test_library(lib_name):
    # Create reference dumps for integration tests
    make_cmd = ['make', '-j', lib_name]
    subprocess.check_call(make_cmd, cwd=AOSP_DIR)

def find_lib_lsdump(lib_name, target_arch, target_arch_variant,
                    target_cpu_variant):
    """ Find the lsdump corresponding to lib_name for the given arch parameters
        if it exists"""
    assert 'ANDROID_PRODUCT_OUT' in os.environ
    cpu_variant = '_' + target_cpu_variant
    arch_variant = '_' + target_arch_variant

    if target_cpu_variant == 'generic' or target_cpu_variant is None or target_cpu_variant == '':
        cpu_variant = ''
    if target_arch_variant == target_arch or target_arch_variant is None or target_arch_variant == '':
        arch_variant = ''

    target_dir = 'android_' + target_arch + arch_variant +\
    cpu_variant + '_shared_core'
    soong_dir = os.path.join(AOSP_DIR, 'out', 'soong', '.intermediates')
    expected_lsdump_name = lib_name + SOURCE_ABI_DUMP_EXT
    for base, dirnames, filenames in os.walk(soong_dir):
        for filename in filenames:
            if filename == expected_lsdump_name:
                path = os.path.join(base, filename)
                if target_dir in os.path.dirname(path):
                    return path
    return None

def run_abi_diff(new_test_dump_path, old_test_dump_path, arch, lib_name,
                 flags=[]):
    abi_diff_cmd = ['header-abi-diff', '-new', new_test_dump_path, '-old',
                     old_test_dump_path, '-arch', arch, '-lib', lib_name]
    with tempfile.TemporaryDirectory() as tmp:
        output_name = os.path.join(tmp, lib_name) + '.abidiff'
        abi_diff_cmd += ['-o', output_name]
        abi_diff_cmd += flags
        try:
            subprocess.check_call(abi_diff_cmd)
        except subprocess.CalledProcessError as err:
            return err.returncode

    return 0


def get_build_var(name):
    """Get build system variable for the launched target."""
    if 'ANDROID_PRODUCT_OUT' not in os.environ:
        return None

    cmd = ['make', '--no-print-directory', '-f', 'build/core/config.mk',
           'dumpvar-' + name]

    environ = dict(os.environ)
    environ['CALLED_FROM_SETUP'] = 'true'
    environ['BUILD_SYSTEM'] = 'build/core'

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, env=environ,
                            cwd=AOSP_DIR)
    out, err = proc.communicate()
    return out.decode('utf-8').strip()

#!/usr/bin/env python3

import os
import unittest

from utils import run_header_checker
from utils import run_abi_diff
from utils import SOURCE_ABI_DUMP_EXT
from utils import get_build_var
from utils import make_test_library
from utils import find_lib_lsdump

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
INPUT_DIR = os.path.join(SCRIPT_DIR, 'input')
EXPECTED_DIR = os.path.join(SCRIPT_DIR, 'expected')
REF_DUMP_DIR = os.path.join(SCRIPT_DIR,  'reference_dumps')

class MyTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.maxDiff = None
        #TODO: Make the test frame-work support multiple arch configurations not
        # just 2
        cls.target_arch =  get_build_var('TARGET_ARCH')
        cls.target_arch_variant =  get_build_var('TARGET_ARCH_VARIANT')
        cls.target_cpu_variant =  get_build_var('TARGET_CPU_VARIANT')

        cls.target_2nd_arch =  get_build_var('TARGET_2ND_ARCH')
        cls.target_2nd_arch_variant =  get_build_var('TARGET_2ND_ARCH_VARIANT')
        cls.target_2nd_cpu_variant =  get_build_var('TARGET_2ND_CPU_VARIANT')

    def run_and_compare(self, input_path, expected_path, cflags=[]):
        with open(expected_path, 'r') as f:
            expected_output = f.read()
        actual_output = run_header_checker(input_path, cflags)
        self.assertEqual(actual_output, expected_output)

    def run_and_compare_name(self, name, cflags=[]):
        input_path = os.path.join(INPUT_DIR, name)
        expected_path = os.path.join(EXPECTED_DIR, name)
        self.run_and_compare(input_path, expected_path, cflags)

    def run_and_compare_name_cpp(self, name, cflags=[]):
        self.run_and_compare_name(name, cflags + ['-x', 'c++', '-std=c++11'])

    def run_and_compare_name_c_cpp(self, name, cflags=[]):
        self.run_and_compare_name(name, cflags)
        self.run_and_compare_name_cpp(name, cflags)

    def run_and_compare_abi_diff(self, new_dump, old_dump, lib, arch,
                                 expected_return_code, flags=[]) :
      actual_output = run_abi_diff(new_dump, old_dump, arch, lib, flags)
      self.assertEqual(actual_output, expected_return_code)

    def prepare_and_run_abi_diff(self, old_lib_name, new_lib_name, target_arch,
                                 target_arch_variant, target_cpu_variant,
                                 expected_return_code, flags=[]):
        make_test_library(new_lib_name)
        arch_lsdump_path = find_lib_lsdump(new_lib_name, target_arch,
                                           target_arch_variant,
                                           target_cpu_variant)
        arch_ref_dump_dir = os.path.join(REF_DUMP_DIR, target_arch)
        arch_ref_dump_path = os.path.join(arch_ref_dump_dir,
                                          old_lib_name + SOURCE_ABI_DUMP_EXT)

        self.run_and_compare_abi_diff(arch_lsdump_path, arch_ref_dump_path,
                                      new_lib_name, self.target_arch,
                                      expected_return_code, flags)

    def prepare_and_run_abi_diff_both_archs(self, old_lib_name, new_lib_name,
                                            expected_return_code, flags=[]):
        if self.target_arch != '':
            self.prepare_and_run_abi_diff(old_lib_name, new_lib_name,
                                          self.target_arch,
                                          self.target_arch_variant,
                                          self.target_cpu_variant,
                                          expected_return_code, flags)

        if self.target_2nd_arch != '':
            self.prepare_and_run_abi_diff(old_lib_name, new_lib_name,
                                          self.target_2nd_arch,
                                          self.target_2nd_arch_variant,
                                          self.target_2nd_cpu_variant,
                                          expected_return_code, flags)

    def test_func_decl_no_args(self):
        self.run_and_compare_name_c_cpp('func_decl_no_args.h')

    def test_func_decl_one_arg(self):
        self.run_and_compare_name_c_cpp('func_decl_one_arg.h')

    def test_func_decl_two_args(self):
        self.run_and_compare_name_c_cpp('func_decl_two_args.h')

    def test_func_decl_one_arg_ret(self):
        self.run_and_compare_name_c_cpp('func_decl_one_arg_ret.h')

    def test_example1(self):
        self.run_and_compare_name_cpp('example1.h')
        self.run_and_compare_name_cpp('example2.h')

    def test_libc_and_cpp(self):
        self.prepare_and_run_abi_diff_both_archs("libc_and_cpp", "libc_and_cpp",
                                                 0)

    def test_libc_and_cpp_and_libc_and_cpp_whole_static(self):
        self.prepare_and_run_abi_diff_both_archs("libc_and_cpp",
                                                 "libc_and_cpp_whole_static",
                                                 0)

    def test_libc_and_cpp_and_libc_and_cpp_with_unused_struct(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libc_and_cpp", "libc_and_cpp_with_unused_struct", 0)

    def test_libc_and_cpp_and_libc_and_cpp_with_unused_struct_check_all(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libc_and_cpp", "libc_and_cpp_with_unused_struct", 1,
            ['-check-all-apis'])

    def test_libc_and_cpp_and_libc_and_cpp_with_unused_struct_check_all_advice(
        self):
        self.prepare_and_run_abi_diff_both_archs(
            "libc_and_cpp", "libc_and_cpp_with_unused_struct", 0,
            ['-check-all-apis', '-advice-only'])

    def test_libgolden_cpp_return_type_diff(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libgolden_cpp", "libgolden_cpp_return_type_diff", 8)

    def test_libgolden_cpp_add_function(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libgolden_cpp", "libgolden_cpp_add_function", 4)

    def test_libgolden_cpp_change_function_access(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libgolden_cpp", "libgolden_cpp_change_function_access", 8)

    def test_libgolden_cpp_add_global_variable(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libgolden_cpp", "libgolden_cpp_add_global_variable", 4)

    def test_libgolden_cpp_parameter_type_diff(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libgolden_cpp", "libgolden_cpp_parameter_type_diff", 8)


    def test_libgolden_cpp_with_vtable_diff(self):
        self.prepare_and_run_abi_diff_both_archs("libgolden_cpp",
                                                 "libgolden_cpp_vtable_diff",
                                                 8)

    def test_libgolden_cpp_member_diff_advice_only(self):
        self.prepare_and_run_abi_diff_both_archs("libgolden_cpp",
                                                 "libgolden_cpp_member_diff",
                                                 0, ['-advice-only'])

    def test_libgolden_cpp_member_diff(self):
        self.prepare_and_run_abi_diff_both_archs("libgolden_cpp",
                                                 "libgolden_cpp_member_diff",
                                                 8)

    def test_libgolden_cpp_change_member_access(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libgolden_cpp", "libgolden_cpp_change_member_access", 8)

    def test_libgolden_cpp_enum_extended(self):
        self.prepare_and_run_abi_diff_both_archs("libgolden_cpp",
                                                 "libgolden_cpp_enum_extended",
                                                 4)
    def test_libgolden_cpp_enum_diff(self):
        self.prepare_and_run_abi_diff_both_archs("libgolden_cpp",
                                                 "libgolden_cpp_enum_diff",
                                                 8)

    def test_libgolden_cpp_fabricated_function_ast_removed_diff(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libgolden_cpp_fabricated_function_ast_removed", "libgolden_cpp",
            0)

    def test_libgolden_cpp_member_fake_diff(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libgolden_cpp", "libgolden_cpp_member_fake_diff",
            0)

    def test_libgolden_cpp_member_integral_type_diff(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libgolden_cpp", "libgolden_cpp_member_integral_type_diff",
            8)

    def test_libgolden_cpp_member_cv_diff(self):
        self.prepare_and_run_abi_diff_both_archs(
            "libgolden_cpp", "libgolden_cpp_member_cv_diff",
            8)

if __name__ == '__main__':
    unittest.main()

#!/usr/bin/env python3

from __future__ import print_function

import argparse
import collections
import os
import re
import stat
import struct
import sys


#------------------------------------------------------------------------------
# Python 2 and 3 Compatibility Layer
#------------------------------------------------------------------------------

if sys.version_info >= (3, 0):
    from os import makedirs
    from mmap import ACCESS_READ, mmap
else:
    from mmap import ACCESS_READ, mmap

    def makedirs(path, exist_ok):
        if exist_ok and os.path.isdir(path):
            return
        return os.makedirs(path)

    class mmap(mmap):
        def __enter__(self):
            return self

        def __exit__(self, exc, value, tb):
            self.close()

        def __getitem__(self, key):
            res = super(mmap, self).__getitem__(key)
            if type(key) == int:
                return ord(res)
            return res

    FileNotFoundError = OSError

try:
    from sys import intern
except ImportError:
    pass


#------------------------------------------------------------------------------
# ELF Parser
#------------------------------------------------------------------------------

Elf_Hdr = collections.namedtuple(
        'Elf_Hdr',
        'ei_class ei_data ei_version ei_osabi e_type e_machine e_version '
        'e_entry e_phoff e_shoff e_flags e_ehsize e_phentsize e_phnum '
        'e_shentsize e_shnum e_shstridx')


Elf_Shdr = collections.namedtuple(
        'Elf_Shdr',
        'sh_name sh_type sh_flags sh_addr sh_offset sh_size sh_link sh_info '
        'sh_addralign sh_entsize')


Elf_Dyn = collections.namedtuple('Elf_Dyn', 'd_tag d_val')


class Elf_Sym(collections.namedtuple(
    'ELF_Sym', 'st_name st_value st_size st_info st_other st_shndx')):

    STB_LOCAL = 0
    STB_GLOBAL = 1
    STB_WEAK = 2

    SHN_UNDEF = 0

    @property
    def st_bind(self):
        return (self.st_info >> 4)

    @property
    def is_local(self):
        return self.st_bind == Elf_Sym.STB_LOCAL

    @property
    def is_global(self):
        return self.st_bind == Elf_Sym.STB_GLOBAL

    @property
    def is_weak(self):
        return self.st_bind == Elf_Sym.STB_WEAK

    @property
    def is_undef(self):
        return self.st_shndx == Elf_Sym.SHN_UNDEF


class ELFError(ValueError):
    pass


class ELF(object):
    # ELF file format constants.
    ELF_MAGIC = b'\x7fELF'

    EI_CLASS = 4
    EI_DATA = 5

    ELFCLASSNONE = 0
    ELFCLASS32 = 1
    ELFCLASS64 = 2

    ELFDATANONE = 0
    ELFDATA2LSB = 1
    ELFDATA2MSB = 2

    DT_NEEDED = 1
    DT_RPATH = 15
    DT_RUNPATH = 29

    _ELF_CLASS_NAMES = {
        ELFCLASS32: '32',
        ELFCLASS64: '64',
    }

    _ELF_DATA_NAMES = {
        ELFDATA2LSB: 'Little-Endian',
        ELFDATA2MSB: 'Big-Endian',
    }

    _ELF_MACHINE_IDS = {
        0: 'EM_NONE',
        3: 'EM_386',
        8: 'EM_MIPS',
        40: 'EM_ARM',
        62: 'EM_X86_64',
        183: 'EM_AARCH64',
    }


    __slots__ = ('ei_class', 'ei_data', 'e_machine', 'dt_rpath', 'dt_runpath',
                 'dt_needed', 'exported_symbols', 'imported_symbols',)


    def __init__(self, ei_class=ELFCLASSNONE, ei_data=ELFDATANONE, e_machine=0,
                 dt_rpath=None, dt_runpath=None, dt_needed=None,
                 exported_symbols=None, imported_symbols=None):
        self.ei_class = ei_class
        self.ei_data = ei_data
        self.e_machine = e_machine
        self.dt_rpath = dt_rpath if dt_rpath is not None else []
        self.dt_runpath = dt_runpath if dt_runpath is not None else []
        self.dt_needed = dt_needed if dt_needed is not None else []
        self.exported_symbols = \
                exported_symbols if exported_symbols is not None else set()
        self.imported_symbols = \
                imported_symbols if imported_symbols is not None else set()

    def __repr__(self):
        args = (a + '=' + repr(getattr(self, a)) for a in self.__slots__)
        return 'ELF(' + ', '.join(args) + ')'

    def __eq__(self, rhs):
        return all(getattr(self, a) == getattr(rhs, a) for a in self.__slots__)

    @property
    def elf_class_name(self):
        return self._ELF_CLASS_NAMES.get(self.ei_class, 'None')

    @property
    def elf_data_name(self):
        return self._ELF_DATA_NAMES.get(self.ei_data, 'None')

    @property
    def elf_machine_name(self):
        return self._ELF_MACHINE_IDS.get(self.e_machine, str(self.e_machine))

    @property
    def is_32bit(self):
        return self.ei_class == ELF.ELFCLASS32

    @property
    def is_64bit(self):
        return self.ei_class == ELF.ELFCLASS64

    @property
    def sorted_exported_symbols(self):
        return sorted(list(self.exported_symbols))

    @property
    def sorted_imported_symbols(self):
        return sorted(list(self.imported_symbols))

    def dump(self, file=None):
        """Print parsed ELF information to the file"""
        file = file if file is not None else sys.stdout

        print('EI_CLASS\t' + self.elf_class_name, file=file)
        print('EI_DATA\t\t' + self.elf_data_name, file=file)
        print('E_MACHINE\t' + self.elf_machine_name, file=file)
        for dt_rpath in self.dt_rpath:
            print('DT_RPATH\t' + dt_rpath, file=file)
        for dt_runpath in self.dt_runpath:
            print('DT_RUNPATH\t' + dt_runpath, file=file)
        for dt_needed in self.dt_needed:
            print('DT_NEEDED\t' + dt_needed, file=file)
        for symbol in self.sorted_exported_symbols:
            print('EXP_SYMBOL\t' + symbol, file=file)
        for symbol in self.sorted_imported_symbols:
            print('IMP_SYMBOL\t' + symbol, file=file)

    def dump_exported_symbols(self, file=None):
        """Print exported symbols to the file"""
        file = file if file is not None else sys.stdout
        for symbol in self.sorted_exported_symbols:
            print(symbol, file=file)

    # Extract zero-terminated buffer slice.
    def _extract_zero_terminated_buf_slice(self, buf, offset):
        """Extract a zero-terminated buffer slice from the given offset"""
        end = offset
        try:
            while buf[end] != 0:
                end += 1
        except IndexError:
            pass
        return buf[offset:end]

    # Extract c-style interned string from the buffer.
    if sys.version_info >= (3, 0):
        def _extract_zero_terminated_str(self, buf, offset):
            """Extract a c-style string from the given buffer and offset"""
            buf_slice = self._extract_zero_terminated_buf_slice(buf, offset)
            return intern(buf_slice.decode('utf-8'))
    else:
        def _extract_zero_terminated_str(self, buf, offset):
            """Extract a c-style string from the given buffer and offset"""
            return intern(self._extract_zero_terminated_buf_slice(buf, offset))

    def _parse_from_buf_internal(self, buf):
        """Parse ELF image resides in the buffer"""

        # Check ELF ident.
        if buf.size() < 8:
            raise ELFError('bad ident')

        if buf[0:4] != ELF.ELF_MAGIC:
            raise ELFError('bad magic')

        self.ei_class = buf[ELF.EI_CLASS]
        if self.ei_class not in (ELF.ELFCLASS32, ELF.ELFCLASS64):
            raise ELFError('unknown word size')

        self.ei_data = buf[ELF.EI_DATA]
        if self.ei_data not in (ELF.ELFDATA2LSB, ELF.ELFDATA2MSB):
            raise ELFError('unknown endianness')

        # ELF structure definitions.
        endian_fmt = '<' if self.ei_data == ELF.ELFDATA2LSB else '>'

        if self.is_32bit:
            elf_hdr_fmt = endian_fmt + '4x4B8xHHLLLLLHHHHHH'
            elf_shdr_fmt = endian_fmt + 'LLLLLLLLLL'
            elf_dyn_fmt = endian_fmt + 'lL'
            elf_sym_fmt = endian_fmt + 'LLLBBH'
        else:
            elf_hdr_fmt = endian_fmt + '4x4B8xHHLQQQLHHHHHH'
            elf_shdr_fmt = endian_fmt + 'LLQQQQLLQQ'
            elf_dyn_fmt = endian_fmt + 'QQ'
            elf_sym_fmt = endian_fmt + 'LBBHQQ'

        def parse_struct(cls, fmt, offset, error_msg):
            try:
                return cls._make(struct.unpack_from(fmt, buf, offset))
            except struct.error:
                raise ELFError(error_msg)

        def parse_elf_hdr(offset):
            return parse_struct(Elf_Hdr, elf_hdr_fmt, offset, 'bad elf header')

        def parse_elf_shdr(offset):
            return parse_struct(Elf_Shdr, elf_shdr_fmt, offset,
                                'bad section header')

        def parse_elf_dyn(offset):
            return parse_struct(Elf_Dyn, elf_dyn_fmt, offset,
                                'bad .dynamic entry')

        if self.is_32bit:
            def parse_elf_sym(offset):
                return parse_struct(Elf_Sym, elf_sym_fmt, offset, 'bad elf sym')
        else:
            def parse_elf_sym(offset):
                try:
                    p = struct.unpack_from(elf_sym_fmt, buf, offset)
                    return Elf_Sym(p[0], p[4], p[5], p[1], p[2], p[3])
                except struct.error:
                    raise ELFError('bad elf sym')

        def extract_str(offset):
            return self._extract_zero_terminated_str(buf, offset)

        # Parse ELF header.
        header = parse_elf_hdr(0)
        self.e_machine = header.e_machine

        # Check section header size.
        if header.e_shentsize == 0:
            raise ELFError('no section header')

        # Find .shstrtab section.
        shstrtab_shdr_off = \
                header.e_shoff + header.e_shstridx * header.e_shentsize
        shstrtab_shdr = parse_elf_shdr(shstrtab_shdr_off)
        shstrtab_off = shstrtab_shdr.sh_offset

        # Parse ELF section header.
        sections = dict()
        header_end = header.e_shoff + header.e_shnum * header.e_shentsize
        for shdr_off in range(header.e_shoff, header_end, header.e_shentsize):
            shdr = parse_elf_shdr(shdr_off)
            name = extract_str(shstrtab_off + shdr.sh_name)
            sections[name] = shdr

        # Find .dynamic and .dynstr section header.
        dynamic_shdr = sections.get('.dynamic')
        if not dynamic_shdr:
            raise ELFError('no .dynamic section')

        dynstr_shdr = sections.get('.dynstr')
        if not dynstr_shdr:
            raise ELFError('no .dynstr section')

        dynamic_off = dynamic_shdr.sh_offset
        dynstr_off = dynstr_shdr.sh_offset

        # Parse entries in .dynamic section.
        assert struct.calcsize(elf_dyn_fmt) == dynamic_shdr.sh_entsize
        dynamic_end = dynamic_off + dynamic_shdr.sh_size
        for ent_off in range(dynamic_off, dynamic_end, dynamic_shdr.sh_entsize):
            ent = parse_elf_dyn(ent_off)
            if ent.d_tag == ELF.DT_NEEDED:
                self.dt_needed.append(extract_str(dynstr_off + ent.d_val))
            elif ent.d_tag == ELF.DT_RPATH:
                self.dt_rpath.extend(
                        extract_str(dynstr_off + ent.d_val).split(':'))
            elif ent.d_tag == ELF.DT_RUNPATH:
                self.dt_runpath.extend(
                        extract_str(dynstr_off + ent.d_val).split(':'))

        # Parse exported symbols in .dynsym section.
        dynsym_shdr = sections.get('.dynsym')
        if dynsym_shdr:
            exp_symbols = self.exported_symbols
            imp_symbols = self.imported_symbols

            dynsym_off = dynsym_shdr.sh_offset
            dynsym_end = dynsym_off + dynsym_shdr.sh_size
            dynsym_entsize = dynsym_shdr.sh_entsize

            # Skip first symbol entry (null symbol).
            dynsym_off += dynsym_entsize

            for ent_off in range(dynsym_off, dynsym_end, dynsym_entsize):
                ent = parse_elf_sym(ent_off)
                symbol_name = extract_str(dynstr_off + ent.st_name)
                if ent.is_undef:
                    imp_symbols.add(symbol_name)
                elif not ent.is_local:
                    exp_symbols.add(symbol_name)

    def _parse_from_buf(self, buf):
        """Parse ELF image resides in the buffer"""
        try:
            self._parse_from_buf_internal(buf)
        except IndexError:
            raise ELFError('bad offset')

    def _parse_from_file(self, path):
        """Parse ELF image from the file path"""
        with open(path, 'rb') as f:
            st = os.fstat(f.fileno())
            if not st.st_size:
                raise ELFError('empty file')
            with mmap(f.fileno(), st.st_size, access=ACCESS_READ) as image:
                self._parse_from_buf(image)

    @staticmethod
    def load(path):
        """Create an ELF instance from the file path"""
        elf = ELF()
        elf._parse_from_file(path)
        return elf

    @staticmethod
    def loads(buf):
        """Create an ELF instance from the buffer"""
        elf = ELF()
        elf._parse_from_buf(buf)
        return elf


#------------------------------------------------------------------------------
# NDK and Banned Libraries
#------------------------------------------------------------------------------

class NDKLibDict(object):
    LLNDK_LIB_NAMES = (
        'libc.so',
        'libdl.so',
        'liblog.so',
        'libm.so',
        'libstdc++.so',
        'libz.so',
    )

    SPNDK_LIB_NAMES = (
        'libEGL.so',
        'libGLESv1_CM.so',
        'libGLESv2.so',
        'libGLESv3.so',
    )

    HLNDK_LIB_NAMES = (
        'libOpenMAXAL.so',
        'libOpenSLES.so',
        'libandroid.so',
        'libcamera2ndk.so',
        'libjnigraphics.so',
        'libmediandk.so',
        'libvulkan.so',
    )

    @staticmethod
    def _compile_path_matcher(names):
        patts = '|'.join('(?:^\\/system\\/lib(?:64)?\\/' + re.escape(i) + '$)'
                         for i in names)
        return re.compile(patts)

    def __init__(self):
        self.llndk_patterns = self._compile_path_matcher(self.LLNDK_LIB_NAMES)
        self.spndk_patterns = self._compile_path_matcher(self.SPNDK_LIB_NAMES)
        self.hlndk_patterns = self._compile_path_matcher(self.HLNDK_LIB_NAMES)
        self.ndk_patterns = self._compile_path_matcher(
                self.LLNDK_LIB_NAMES + self.SPNDK_LIB_NAMES +
                self.HLNDK_LIB_NAMES)

    def is_ndk(self, path):
        return self.ndk_patterns.match(path)

    def is_llndk(self, path):
        return self.llndk_patterns.match(path)

    def is_spndk(self, path):
        return self.spndk_patterns.match(path)

    def is_hlndk(self, path):
        return self.hlndk_patterns.match(path)

NDK_LIBS = NDKLibDict()


BannedLib = collections.namedtuple(
        'BannedLib', ('name', 'reason', 'action',))

BA_WARN = 0
BA_EXCLUDE = 1

class BannedLibDict(object):
    def __init__(self):
        self.banned_libs = dict()

    def add(self, name, reason, action):
        self.banned_libs[name] = BannedLib(name, reason, action)

    def get(self, name):
        return self.banned_libs.get(name)

    def is_banned(self, path):
        return self.get(os.path.basename(path))

    @staticmethod
    def create_default():
        d = BannedLibDict()
        d.add('libbinder.so', 'un-versioned IPC', BA_WARN)
        d.add('libselinux.so', 'policydb might be incompatible', BA_WARN)
        return d


#------------------------------------------------------------------------------
# ELF Linker
#------------------------------------------------------------------------------

def is_accessible(path):
    try:
        mode = os.stat(path).st_mode
        return (mode & (stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)) != 0
    except FileNotFoundError:
        return False


def scan_executables(root):
    for base, dirs, files in os.walk(root):
        for filename in files:
            path = os.path.join(base, filename)
            if is_accessible(path):
                yield path


PT_SYSTEM = 0
PT_VENDOR = 1
NUM_PARTITIONS = 2


VNDKHeuristics = collections.namedtuple(
        'VNDKHeuristics',
        'extra_system_libs extra_vendor_libs extra_vndk_core '
        'vndk_core vndk_indirect vndk_fwk_ext vndk_vnd_ext')


class ELFResolver(object):
    def __init__(self, lib_set, default_search_path):
        self.lib_set = lib_set
        self.default_search_path = default_search_path

    def get_candidates(self, name, dt_rpath=None, dt_runpath=None):
        if dt_rpath:
            for d in dt_rpath:
                yield os.path.join(d, name)
        if dt_runpath:
            for d in dt_runpath:
                yield os.path.join(d, name)
        for d in self.default_search_path:
            yield os.path.join(d, name)

    def resolve(self, name, dt_rpath=None, dt_runpath=None):
        for path in self.get_candidates(name, dt_rpath, dt_runpath):
            try:
                return self.lib_set[path]
            except KeyError:
                continue
        return None


class ELFLinkData(object):
    def __init__(self, partition, path, elf):
        self.partition = partition
        self.path = path
        self.elf = elf
        self.deps = set()
        self.users = set()
        self.extended_symbol_users = set()
        self.is_ndk = NDK_LIBS.is_ndk(path)
        self.unresolved_symbols = set()
        self.linked_symbols = dict()

    def add_dep(self, dst):
        self.deps.add(dst)
        dst.users.add(self)

    def remove_dep(self, dst):
        self.deps.remove(dst)
        dst.users.remove(self)

    def is_system_lib(self):
        return self.partition == PT_SYSTEM

_VNDK_EXT_PATTERN = \
        re.compile('^/(?:(?:system)|(?:vendor))/lib(?:64)?/vndk-\\d+-ext/')

_VNDK_VERSION = 26  # FIXME: This should be user-specified argument.

def is_vndk_ext_path(path):
    return _VNDK_EXT_PATTERN.match(path)

def convert_path_to_vndk_core(vndk_version, path):
    filename = os.path.basename(path)
    if 'lib64' in path:
        return '/system/lib64/vndk-{}/{}'.format(vndk_version, filename)
    else:
        return '/system/lib/vndk-{}/{}'.format(vndk_version, filename)

def sorted_lib_path_list(libs):
    libs = [lib.path for lib in libs]
    libs.sort()
    return libs


class ELFLinker(object):
    def __init__(self):
        self.lib32 = dict()
        self.lib64 = dict()
        self.lib_pt = [dict() for i in range(NUM_PARTITIONS)]

    def add(self, partition, path, elf):
        node = ELFLinkData(partition, path, elf)
        if elf.is_32bit:
            self.lib32[path] = node
        else:
            self.lib64[path] = node
        self.lib_pt[partition][path] = node

    def add_dep(self, src_path, dst_path):
        for lib_set in (self.lib32, self.lib64):
            src = lib_set.get(src_path)
            dst = lib_set.get(dst_path)
            if src and dst:
                src.add_dep(dst)

    def map_path_to_lib(self, path):
        for lib_set in (self.lib32, self.lib64):
            lib = lib_set.get(path)
            if lib:
                return lib
        return None

    def map_paths_to_libs(self, paths, report_error):
        result = set()
        for path in paths:
            lib = self.map_path_to_lib(path)
            if not lib:
                report_error(path)
                continue
            result.add(lib)
        return result

    @staticmethod
    def _compile_path_matcher(root, subdirs):
        dirs = [os.path.normpath(os.path.join(root, i)) for i in subdirs]
        patts = ['(?:' + re.escape(i) + os.sep + ')' for i in dirs]
        return re.compile('|'.join(patts))

    def add_executables_in_dir(self, partition_name, partition, root,
                               alter_partition, alter_subdirs):
        root = os.path.abspath(root)
        prefix_len = len(root) + 1

        if alter_subdirs:
            alter_patt = ELFLinker._compile_path_matcher(root, alter_subdirs)

        for path in scan_executables(root):
            try:
                elf = ELF.load(path)
            except ELFError as e:
                continue

            short_path = os.path.join('/', partition_name, path[prefix_len:])
            if alter_subdirs and alter_patt.match(path):
                self.add(alter_partition, short_path, elf)
            else:
                self.add(partition, short_path, elf)

    def load_extra_deps(self, path):
        patt = re.compile('([^:]*):\\s*(.*)')
        with open(path, 'r') as f:
            for line in f:
                match = patt.match(line)
                if match:
                    self.add_dep(match.group(1), match.group(2))

    def _find_exported_symbol(self, symbol, libs):
        """Find the shared library with the exported symbol."""
        for lib in libs:
            if symbol in lib.elf.exported_symbols:
                return lib
        return None

    def _resolve_lib_imported_symbols(self, lib, imported_libs):
        """Resolve the imported symbols in a library."""
        for symbol in lib.elf.imported_symbols:
            imported_lib = self._find_exported_symbol(symbol, imported_libs)
            if imported_lib:
                lib.linked_symbols[symbol] = imported_lib
            else:
                lib.unresolved_symbols.add(symbol)

    def _resolve_lib_dt_needed(self, lib, resolver):
        imported_libs = []
        for dt_needed in lib.elf.dt_needed:
            dep = resolver.resolve(dt_needed, lib.elf.dt_rpath,
                                   lib.elf.dt_runpath)
            if not dep:
                candidates = list(resolver.get_candidates(
                    dt_needed, lib.elf.dt_rpath, lib.elf.dt_runpath))
                print('warning: {}: Missing needed library: {}  Tried: {}'
                      .format(lib.path, dt_needed, candidates), file=sys.stderr)
                continue
            lib.add_dep(dep)
            imported_libs.append(dep)
        return imported_libs

    def _resolve_lib_deps(self, lib, resolver):
        imported_libs = self._resolve_lib_dt_needed(lib, resolver)
        self._resolve_lib_imported_symbols(lib, imported_libs)

    def _resolve_lib_set_deps(self, lib_set, resolver):
        for lib in lib_set.values():
            self._resolve_lib_deps(lib, resolver)

    def resolve_deps(self):
        self._resolve_lib_set_deps(
                self.lib32,
                ELFResolver(self.lib32, ['/system/lib', '/vendor/lib']))

        self._resolve_lib_set_deps(
                self.lib64,
                ELFResolver(self.lib64, ['/system/lib64', '/vendor/lib64']))

    def _resolve_lib_extended_symbol_users(self, generic_refs, lib):
        """Resolve the users of the extended exported symbols of a library."""
        try:
            ref_lib = generic_refs.refs[lib.path]
        except KeyError:
            lib.extended_symbol_users = lib.users
            return

        for user in lib.users:
            for symbol, imp_lib in user.linked_symbols.items():
                if imp_lib is not lib:
                    continue
                if symbol not in ref_lib:
                    lib.extended_symbol_users.add(user)

    def resolve_extended_symbol_users(self, generic_refs):
        """Resolve the users of the extended exported symbols."""
        for lib_set in self.lib_pt:
            for lib in lib_set.values():
                self._resolve_lib_extended_symbol_users(generic_refs, lib)

    def compute_matched_libs(self, path_patterns, closure=False,
                             is_excluded_libs=None):
        patt = re.compile('|'.join('(?:' + p + ')' for p in path_patterns))

        # Find libraries with matching paths.
        libs = set()
        for lib_set in self.lib_pt:
            for lib in lib_set.values():
                if patt.match(lib.path):
                    libs.add(lib)

        if closure:
            # Compute transitive closure.
            if not is_excluded_libs:
                def is_excluded_libs(lib):
                    return False
            libs = self.compute_closure(libs, is_excluded_libs)

        return libs

    def compute_vndk_stable(self, closure):
        """Find all vndk stable libraries."""

        path_patterns = (
            # HIDL libraries used by android.hardware.graphics.mapper@2.0-impl.
            '^.*/libhidlbase\\.so$',
            '^.*/libhidltransport\\.so$',
            '^.*/libhidlmemory\\.so$',
            '^.*/libfmp\\.so$',
            '^.*/libhwbinder\\.so$',

            # UI libraries used by libEGL.
            #'^.*/libui\\.so$',
            #'^.*/libnativewindow\\.so$',
        )

        def is_excluded_libs(lib):
            return lib.is_ndk

        return self.compute_matched_libs(path_patterns, closure,
                                         is_excluded_libs)

    def compute_sp_hal(self, vndk_stable, closure):
        """Find all same-process HALs."""

        path_patterns = (
            # OpenGL-related
            '^/vendor/.*/libEGL_.*\\.so$',
            '^/vendor/.*/libGLESv1_CM_.*\\.so$',
            '^/vendor/.*/libGLESv2_.*\\.so$',
            '^/vendor/.*/libGLESv3_.*\\.so$',
            # Vulkan
            '^/vendor/.*/vulkan.*\\.so$',
            # libRSDriver
            '^/vendor/.*/libRSDriver.*\\.so$',
            '^/vendor/.*/libPVRRS\\.so$',
            # Gralloc mapper
            '^.*/gralloc\\..*\\.so$',
            '^.*/android\\.hardware\\.graphics\\.mapper@\\d+\\.\\d+-impl\\.so$',
        )

        def is_excluded_libs(lib):
            return lib.is_ndk or lib in vndk_stable

        return self.compute_matched_libs(path_patterns, closure,
                                         is_excluded_libs)

    def _po_sorted(self, lib_set, get_assoc_libs):
        result = []
        visited = set()
        def traverse(lib):
            for assoc_lib in get_assoc_libs(lib):
                if assoc_lib in lib_set and assoc_lib not in visited:
                    visited.add(assoc_lib)
                    traverse(assoc_lib)
            result.append(lib)
        for lib in lib_set:
            if lib not in visited:
                traverse(lib)
        return result

    def _deps_po_sorted(self, lib_set):
        return self._po_sorted(lib_set, lambda x: x.deps)

    def _users_po_sorted(self, lib_set):
        return self._po_sorted(lib_set, lambda x: x.users)

    def normalize_partition_tags(self, sp_hals, generic_refs):
        system_libs_po = self._deps_po_sorted(self.lib_pt[PT_SYSTEM].values())
        system_libs = self.lib_pt[PT_SYSTEM]
        vendor_libs = self.lib_pt[PT_VENDOR]

        def is_system_lib_or_sp_hal(lib):
            return lib.is_system_lib() or lib in sp_hals

        for lib in system_libs_po:
            if all(is_system_lib_or_sp_hal(dep) for dep in lib.deps):
                # Good system lib.  Do nothing.
                continue
            if not generic_refs or generic_refs.refs.get(lib.path):
                # If lib is in AOSP generic reference, then we assume that the
                # non-SP-HAL dependencies are errors.  Emit errors and remove
                # the dependencies.
                for dep in list(lib.deps):
                    if not is_system_lib_or_sp_hal(dep):
                        print('error: {}: system exe/lib must not depend on '
                              'vendor lib {}.  Assume such dependency does '
                              'not exist.'.format(lib.path, dep.path),
                              file=sys.stderr)
                    lib.remove_dep(dep)
            else:
                # If lib is not in AOSP generic reference, then we assume that
                # lib must be moved to vendor partition.
                for dep in lib.deps:
                    if not is_system_lib_or_sp_hal(dep):
                        print('warning: {}: system exe/lib must not depend on '
                              'vendor lib {}.  Assuming {} should be placed in '
                              'vendor partition.'
                              .format(lib.path, dep.path, lib.path),
                              file=sys.stderr)
                lib.partition = PT_VENDOR
                vendor_libs[lib.path] = lib
                del system_libs[lib.path]

    def compute_vndk(self, sp_hals, vndk_stable, vndk_customized_for_system,
                     vndk_customized_for_vendor, generic_refs, banned_libs):
        # Collect VNDK candidates.
        def is_not_vndk(lib):
            return (lib.is_ndk or banned_libs.is_banned(lib.path) or
                    (lib in sp_hals) or (lib in vndk_stable))

        def collect_libs_with_partition_user(lib_set, partition):
            result = set()
            for lib in lib_set:
                if is_not_vndk(lib):
                    continue
                if any(user.partition == partition for user in lib.users):
                    result.add(lib)
            return result

        vndk_candidates = collect_libs_with_partition_user(
                self.lib_pt[PT_SYSTEM].values(), PT_VENDOR)

        vndk_visited = set(vndk_candidates)

        # Sets for missing libraries.
        extra_vndk_core = set()
        extra_system_libs = set()
        extra_vendor_libs = set()

        # Compute vndk-core, vndk-fwk-ext and vndk-vnd-ext.
        vndk_fwk_ext = set()
        vndk_vnd_ext = set()

        if not generic_refs:
            vndk_core = vndk_candidates
        else:
            vndk_core = set()
            while vndk_candidates:
                prev_vndk_candidates = vndk_candidates
                vndk_candidates = set()

                def add_deps_to_vndk_candidate(lib):
                    for dep in lib.deps:
                        if is_not_vndk(lib):
                            continue
                        if dep.is_system_lib() and dep not in vndk_visited:
                            vndk_candidates.add(dep)
                            vndk_visited.add(dep)

                # Remove non-AOSP libraries.
                vndk_extended_candidates = set()
                vndk_customized_candidates = set()
                for lib in prev_vndk_candidates:
                    category = generic_refs.classify_lib(lib)
                    if category == GenericRefs.NEW_LIB:
                        extra_system_libs.add(lib)
                        extra_vendor_libs.add(lib)
                        add_deps_to_vndk_candidate(lib)
                    elif category == GenericRefs.EXPORT_EQUAL:
                        vndk_customized_candidates.add(lib)
                    elif category == GenericRefs.EXPORT_SUPER_SET:
                        vndk_extended_candidates.add(lib)
                    else:
                        print('error: {}: vndk library must not be modified.'
                              .format(lib.path), file=sys.stderr)

                # Classify VNDK customized candidates.
                for lib in vndk_customized_candidates:
                    if not lib.extended_symbol_users:
                        # Inward-customized VNDK-core libraries.
                        vndk_core.add(lib)
                    else:
                        # Outward-customized VNDK libraries.
                        if lib in vndk_customized_for_system:
                            vndk_fwk_ext.add(lib)
                        if lib in vndk_customized_for_vendor:
                            vndk_vnd_ext.add(lib)
                            add_deps_to_vndk_candidate(lib)

                        # Check whether we have an existing vanilla vndk-core.
                        path = lib.path
                        if is_vndk_ext_path(path):
                            path = convert_path_to_vndk_core(
                                    path, _VNDK_VERSION)
                        vndk_core_lib = self.map_path_to_lib(path)
                        if not vndk_core_lib or vndk_core_lib is lib:
                            extra_vndk_core.add(lib)  # Need a missing vanilla.

                # Compute VNDK extension candidates.
                for lib in self._users_po_sorted(vndk_extended_candidates):
                    has_system_users = False
                    has_vendor_users = False
                    for user in lib.extended_symbol_users:
                        if user.is_system_lib():
                            has_system_users = True
                        else:
                            has_vendor_users = True
                    if has_system_users:
                        vndk_fwk_ext.add(lib)
                    if has_vendor_users:
                        vndk_vnd_ext.add(lib)
                        add_deps_to_vndk_candidate(lib)

        # Compute the closure of the VNDK libs.
        def is_not_vndk_core(lib):
            return (is_not_vndk(lib) or not lib.is_system_lib())

        def is_not_vndk_fwk_ext(lib):
            return (is_not_vndk(lib) or (lib in vndk_core) or
                    (lib in vndk_indirect))

        def is_not_vndk_vnd_ext(lib):
            return (is_not_vndk(lib) or (lib in vndk_core) or
                    (lib in vndk_indirect))

        vndk_indirect = self.compute_closure(vndk_core, is_not_vndk_core)
        vndk_indirect -= vndk_core

        vndk_fwk_ext = self.compute_closure(vndk_fwk_ext, is_not_vndk_fwk_ext)
        vndk_vnd_ext = self.compute_closure(vndk_vnd_ext, is_not_vndk_vnd_ext)

        return VNDKHeuristics(
                extra_system_libs, extra_vendor_libs, extra_vndk_core,
                vndk_core, vndk_indirect, vndk_fwk_ext, vndk_vnd_ext)

    @staticmethod
    def compute_closure(root_set, is_excluded):
        closure = set(root_set)
        stack = list(root_set)
        while stack:
            lib = stack.pop()
            for dep in lib.deps:
                if is_excluded(dep):
                    continue
                if dep not in closure:
                    closure.add(dep)
                    stack.append(dep)
        return closure

    @staticmethod
    def create(system_dirs=None, system_dirs_as_vendor=None, vendor_dirs=None,
               vendor_dirs_as_system=None, extra_deps=None):
        graph = ELFLinker()

        if system_dirs:
            for path in system_dirs:
                graph.add_executables_in_dir('system', PT_SYSTEM, path,
                                             PT_VENDOR, system_dirs_as_vendor)

        if vendor_dirs:
            for path in vendor_dirs:
                graph.add_executables_in_dir('vendor', PT_VENDOR, path,
                                             PT_SYSTEM, vendor_dirs_as_system)

        if extra_deps:
            for path in extra_deps:
                graph.load_extra_deps(path)

        graph.resolve_deps()

        return graph


#------------------------------------------------------------------------------
# Generic Reference
#------------------------------------------------------------------------------

class GenericRefs(object):
    NEW_LIB = 0
    EXPORT_EQUAL = 1
    EXPORT_SUPER_SET = 2
    MODIFIED = 3

    def __init__(self):
        self.refs = dict()

    def add(self, name, symbols):
        self.refs[name] = symbols

    def _load_from_dir(self, root):
        root = os.path.abspath(root)
        prefix_len = len(root) + 1
        for base, dirnames, filenames in os.walk(root):
            for filename in filenames:
                if not filename.endswith('.sym'):
                    continue
                path = os.path.join(base, filename)
                lib_name = '/' + path[prefix_len:-4]
                with open(path, 'r') as f:
                    self.add(lib_name, set(line.strip() for line in f))

    @staticmethod
    def create_from_dir(root):
        result = GenericRefs()
        result._load_from_dir(root)
        return result

    def classify_lib(self, lib):
        ref_lib_symbols = self.refs.get(lib.path)
        if not ref_lib_symbols:
            return GenericRefs.NEW_LIB
        exported_symbols = lib.elf.exported_symbols
        if exported_symbols == ref_lib_symbols:
            return GenericRefs.EXPORT_EQUAL
        if exported_symbols > ref_lib_symbols:
            return GenericRefs.EXPORT_SUPER_SET
        return GenericRefs.MODIFIED

    def is_equivalent_lib(self, lib):
        return self.classify_lib(lib) == GenericRefs.EXPORT_EQUAL


#------------------------------------------------------------------------------
# Commands
#------------------------------------------------------------------------------

class Command(object):
    def __init__(self, name, help):
        self.name = name
        self.help = help

    def add_argparser_options(self, parser):
        pass

    def main(self, args):
        return 0


class ELFDumpCommand(Command):
    def __init__(self):
        super(ELFDumpCommand, self).__init__(
                'elfdump', help='Dump ELF .dynamic section')

    def add_argparser_options(self, parser):
        parser.add_argument('path', help='path to an ELF file')

    def main(self, args):
        try:
            ELF.load(args.path).dump()
        except ELFError as e:
            print('error: {}: Bad ELF file ({})'.format(args.path, e),
                  file=sys.stderr)
            sys.exit(1)
        return 0


class CreateGenericRefCommand(Command):
    def __init__(self):
        super(CreateGenericRefCommand, self).__init__(
                'create-generic-ref', help='Create generic references')

    def add_argparser_options(self, parser):
        parser.add_argument('dir')

        parser.add_argument(
                '--output', '-o', metavar='PATH', required=True,
                help='output directory')

    def main(self, args):
        root = os.path.abspath(args.dir)
        print(root)
        prefix_len = len(root) + 1
        for path in scan_executables(root):
            name = path[prefix_len:]
            try:
                print('Processing:', name, file=sys.stderr)
                elf = ELF.load(path)
                out = os.path.join(args.output, name) + '.sym'
                makedirs(os.path.dirname(out), exist_ok=True)
                with open(out, 'w') as f:
                    elf.dump_exported_symbols(f)
            except ELFError:
                pass
        return 0


class ELFGraphCommand(Command):
    def add_argparser_options(self, parser):
        parser.add_argument(
                '--load-extra-deps', action='append',
                help='load extra module dependencies')

        parser.add_argument(
                '--system', action='append',
                help='path to system partition contents')

        parser.add_argument(
                '--vendor', action='append',
                help='path to vendor partition contents')

        parser.add_argument(
                '--system-dir-as-vendor', action='append',
                help='sub directory of system partition that has vendor files')

        parser.add_argument(
                '--vendor-dir-as-system', action='append',
                help='sub directory of vendor partition that has system files')


class VNDKCommand(ELFGraphCommand):
    def __init__(self):
        super(VNDKCommand, self).__init__(
                'vndk', help='Compute VNDK libraries set')

    def add_argparser_options(self, parser):
        super(VNDKCommand, self).add_argparser_options(parser)

        parser.add_argument(
                '--load-generic-refs',
                help='compare with generic reference symbols')

        parser.add_argument(
                '--warn-incorrect-partition', action='store_true',
                help='warn about libraries only have cross partition linkages')

        parser.add_argument(
                '--warn-high-level-ndk-deps', action='store_true',
                help='warn about VNDK depends on high-level NDK')

        parser.add_argument(
                '--warn-banned-vendor-lib-deps', action='store_true',
                help='warn when a vendor binaries depends on banned lib')

        parser.add_argument(
                '--ban-vendor-lib-dep', action='append',
                help='library that must not be used by vendor binaries')

        parser.add_argument(
                '--outward-customization-default-partition', default='system',
                help='default partition for outward customized vndk libs')

        parser.add_argument(
                '--outward-customization-for-system', action='append',
                help='outward customized vndk for system partition')

        parser.add_argument(
                '--outward-customization-for-vendor', action='append',
                help='outward customized vndk for vendor partition')

    def _warn_incorrect_partition_lib_set(self, lib_set, partition, error_msg):
        for lib in lib_set.values():
            if not lib.users:
                continue
            if all((user.partition != partition for user in lib.users)):
                print(error_msg.format(lib.path), file=sys.stderr)

    def _warn_incorrect_partition(self, graph):
        self._warn_incorrect_partition_lib_set(
                graph.lib_pt[PT_VENDOR], PT_VENDOR,
                'warning: {}: This is a vendor library with framework-only '
                'usages.')

        self._warn_incorrect_partition_lib_set(
                graph.lib_pt[PT_SYSTEM], PT_SYSTEM,
                'warning: {}: This is a framework library with vendor-only '
                'usages.')

    def _warn_high_level_ndk_deps(self, lib_sets):
        for lib_set in lib_sets:
            for lib in lib_set:
                for dep in lib.deps:
                    if NDK_LIBS.is_hlndk(dep.path):
                        print('warning: {}: VNDK is using high-level NDK {}.'
                                .format(lib.path, dep.path), file=sys.stderr)

    def _warn_banned_vendor_lib_deps(self, graph, banned_libs):
        for lib in graph.lib_pt[PT_VENDOR].values():
            for dep in lib.deps:
                banned = banned_libs.is_banned(dep.path)
                if banned:
                    print('warning: {}: Vendor binary depends on banned {} '
                          '(reason: {})'.format(
                              lib.path, dep.path, banned.reason),
                          file=sys.stderr)

    def _check_ndk_extensions(self, graph, generic_refs):
        for lib_set in (graph.lib32, graph.lib64):
            for lib in lib_set.values():
                if lib.is_ndk and not generic_refs.is_equivalent_lib(lib):
                    print('warning: {}: NDK library should not be extended.'
                            .format(lib.path), file=sys.stderr)

    def main(self, args):
        # Link ELF objects.
        graph = ELFLinker.create(args.system, args.system_dir_as_vendor,
                                 args.vendor, args.vendor_dir_as_system,
                                 args.load_extra_deps)

        # Load the generic reference.
        if not args.load_generic_refs:
            generic_refs = None
        else:
            generic_refs = GenericRefs.create_from_dir(args.load_generic_refs)

            # Resolve extended symbol users of each libraries.
            graph.resolve_extended_symbol_users(generic_refs)

            # Check the API extensions to NDK libraries.
            self._check_ndk_extensions(graph, generic_refs)

        # Create banned libraries.
        if not args.ban_vendor_lib_dep:
            banned_libs = BannedLibDict.create_default()
        else:
            banned_libs = BannedLibDict()
            for name in args.ban_vendor_lib_dep:
                banned_libs.add(name, 'user-banned', BA_WARN)

        if args.warn_incorrect_partition:
            self._warn_incorrect_partition(graph)

        if args.warn_banned_vendor_lib_deps:
            self._warn_banned_vendor_lib_deps(graph, banned_libs)

        # Compute sp-hal and vndk-stable.
        vndk_stable = graph.compute_vndk_stable(closure=True)
        sp_hals = graph.compute_sp_hal(vndk_stable, closure=False)
        sp_hals_closure = graph.compute_sp_hal(vndk_stable, closure=True)

        # Normalize partition tags.  We expect many violations from the
        # pre-Treble world.  Guess a resolution for the incorrect partition
        # tag.
        graph.normalize_partition_tags(sp_hals, generic_refs)

        # User may specify the partition for outward-customized vndk libs.  The
        # following code converts the path into ELFLinkData.
        vndk_customized_for_system = set()
        vndk_customized_for_vendor = set()

        system_libs = graph.lib_pt[PT_SYSTEM].values()
        if args.outward_customization_default_partition in {'system', 'both'}:
            vndk_customized_for_system.update(system_libs)

        if args.outward_customization_default_partition in {'vendor', 'both'}:
            vndk_customized_for_vendor.update(system_libs)

        if args.outward_customization_for_system:
            vndk_customized_for_system.update(
                    graph.map_paths_to_libs(
                        args.outward_customization_for_system, lambda x: None))

        if args.outward_customization_for_vendor:
            vndk_customized_for_system.update(
                    graph.map_paths_to_libs(
                        args.outward_customization_for_vendor, lambda x: None))

        # Compute vndk heuristics.
        vndk = graph.compute_vndk(
                sp_hals_closure, vndk_stable, vndk_customized_for_system,
                vndk_customized_for_vendor, generic_refs, banned_libs)

        if args.warn_high_level_ndk_deps:
            self._warn_high_level_ndk_deps(
                    (vndk.extra_vndk_core, vndk.vndk_core, vndk.vndk_indirect,
                     vndk.vndk_fwk_ext, vndk.vndk_vnd_ext))

        for lib in sorted_lib_path_list(sp_hals_closure):
            print('sp-hals:', lib)

        for lib in sorted_lib_path_list(vndk_stable):
            print('vndk-stable:', lib)

        for lib in sorted_lib_path_list(vndk.extra_system_libs):
            print('extra-system-lib:', lib)

        for lib in sorted_lib_path_list(vndk.extra_vendor_libs):
            print('extra-vendor-lib:', lib)

        for lib in sorted_lib_path_list(vndk.extra_vndk_core):
            print('extra-vndk-core:', lib)

        for lib in sorted_lib_path_list(vndk.vndk_core):
            print('vndk-core:', lib)

        for lib in sorted_lib_path_list(vndk.vndk_indirect):
            print('vndk-indirect:', lib)

        for lib in sorted_lib_path_list(vndk.vndk_fwk_ext):
            print('vndk-fwk-ext:', lib)

        for lib in sorted_lib_path_list(vndk.vndk_vnd_ext):
            print('vndk-vnd-ext:', lib)

        return 0


class DepsCommand(ELFGraphCommand):
    def __init__(self):
        super(DepsCommand, self).__init__(
                'deps', help='Print binary dependencies for debugging')

    def add_argparser_options(self, parser):
        super(DepsCommand, self).add_argparser_options(parser)

        parser.add_argument(
                '--revert', action='store_true',
                help='print usage dependency')

        parser.add_argument(
                '--leaf', action='store_true',
                help='print binaries without dependencies or usages')

    def main(self, args):
        graph = ELFLinker.create(args.system, args.system_dir_as_vendor,
                                 args.vendor, args.vendor_dir_as_system,
                                 args.load_extra_deps)

        results = []
        for partition in range(NUM_PARTITIONS):
            for name, lib in graph.lib_pt[partition].items():
                assoc_libs = lib.users if args.revert else lib.deps
                results.append((name, sorted_lib_path_list(assoc_libs)))
        results.sort()

        if args.leaf:
            for name, deps in results:
                if not deps:
                    print(name)
        else:
            for name, deps in results:
                print(name)
                for dep in deps:
                    print('\t' + dep)
        return 0


class DepsClosureCommand(ELFGraphCommand):
    def __init__(self):
        super(DepsClosureCommand, self).__init__(
                'deps-closure', help='Find transitive closure of dependencies')

    def add_argparser_options(self, parser):
        super(DepsClosureCommand, self).add_argparser_options(parser)

        parser.add_argument('lib', nargs='+',
                            help='root set of the shared libraries')

        parser.add_argument('--exclude-lib', action='append', default=[],
                            help='libraries to be excluded')

        parser.add_argument('--exclude-ndk', action='store_true',
                            help='exclude ndk libraries')

    def main(self, args):
        graph = ELFLinker.create(args.system, args.system_dir_as_vendor,
                                 args.vendor, args.vendor_dir_as_system,
                                 args.load_extra_deps)

        # Find root/excluded libraries by their paths.
        def report_error(path):
            print('error: no such lib: {}'.format(path), file=sys.stderr)
        root_libs = graph.map_paths_to_libs(args.lib, report_error)
        excluded_libs = graph.map_paths_to_libs(args.exclude_lib, report_error)

        # Compute and print the closure.
        if args.exclude_ndk:
            def is_excluded_libs(lib):
                return lib.is_ndk or lib in excluded_libs
        else:
            def is_excluded_libs(lib):
                return lib in excluded_libs

        closure = graph.compute_closure(root_libs, is_excluded_libs)
        for lib in sorted_lib_path_list(closure):
            print(lib)
        return 0


class VNDKStableCommand(ELFGraphCommand):
    def __init__(self):
        super(VNDKStableCommand, self).__init__(
                'vndk-stable', help='Find transitive closure of VNDK stable')

    def add_argparser_options(self, parser):
        super(VNDKStableCommand, self).add_argparser_options(parser)

        parser.add_argument('--closure', action='store_true',
                            help='show the closure')

    def main(self, args):
        graph = ELFLinker.create(args.system, args.system_dir_as_vendor,
                                 args.vendor, args.vendor_dir_as_system,
                                 args.load_extra_deps)

        vndk_stable = graph.compute_vndk_stable(closure=args.closure)
        for lib in sorted_lib_path_list(vndk_stable):
            print(lib)
        return 0


class SpHalCommand(ELFGraphCommand):
    def __init__(self):
        super(SpHalCommand, self).__init__(
                'sp-hal', help='Find transitive closure of same-process HALs')

    def add_argparser_options(self, parser):
        super(SpHalCommand, self).add_argparser_options(parser)

        parser.add_argument('--closure', action='store_true',
                            help='show the closure')

    def main(self, args):
        graph = ELFLinker.create(args.system, args.system_dir_as_vendor,
                                 args.vendor, args.vendor_dir_as_system,
                                 args.load_extra_deps)

        vndk_stable = graph.compute_vndk_stable(closure=True)
        sp_hals = graph.compute_sp_hal(vndk_stable, closure=args.closure)
        for lib in sorted_lib_path_list(sp_hals):
            print(lib)
        return 0


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest='subcmd')
    subcmds = dict()

    def register_subcmd(cmd):
        subcmds[cmd.name] = cmd
        cmd.add_argparser_options(
                subparsers.add_parser(cmd.name, help=cmd.help))

    register_subcmd(ELFDumpCommand())
    register_subcmd(CreateGenericRefCommand())
    register_subcmd(VNDKCommand())
    register_subcmd(DepsCommand())
    register_subcmd(DepsClosureCommand())
    register_subcmd(SpHalCommand())
    register_subcmd(VNDKStableCommand())

    args = parser.parse_args()
    if not args.subcmd:
        parser.print_help()
        sys.exit(1)
    return subcmds[args.subcmd].main(args)

if __name__ == '__main__':
    sys.exit(main())

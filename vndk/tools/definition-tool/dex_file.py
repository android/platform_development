#!/usr/bin/env python3

from __future__ import print_function

import argparse
import codecs
import collections
import io
import itertools
import mmap
import os
import struct
import sys
import zipfile


if sys.version_info >= (3, 0):
    from mmap import ACCESS_READ, mmap

    def get_py3_bytes(buf):
        return buf
else:
    from mmap import ACCESS_READ, mmap

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


    class Bytes(bytes):
        def __getitem__(self, key):
            res = super(Bytes, self).__getitem__(key)
            if type(key) == int:
                return ord(res)
            return res

    def get_py3_bytes(buf):
        return Bytes(buf)


if sys.version_info >= (3,):
    create_chr = chr
    enumerate_bytes = enumerate
else:
    create_chr = unichr
    def enumerate_bytes(iterable):
        for i, byte in enumerate(iterable):
            yield (i, ord(byte))


def encode_mutf8(input, errors='strict'):
    i = 0
    res = io.BytesIO()

    for i, char in enumerate(input):
        code = ord(char)
        if code == 0x00:
            res.write(b'\xc0\x80')
        elif code < 0x80:
            res.write(bytearray((code,)))
        elif code < 0x800:
            res.write(bytearray((0xc0 | (code >> 6), 0x80 | (code & 0x3f))))
        elif code < 0x10000:
            res.write(bytearray((0xe0 | (code >> 12),
                                 0x80 | ((code >> 6) & 0x3f),
                                 0x80 | (code & 0x3f))))
        elif code < 0x110000:
            code -= 0x10000
            code_hi = 0xd800 + (code >> 10)
            code_lo = 0xdc00 + (code & 0x3ff)
            res.write(bytearray((0xe0 | (code_hi >> 12),
                                 0x80 | ((code_hi >> 6) & 0x3f),
                                 0x80 | (code_hi & 0x3f),
                                 0xe0 | (code_lo >> 12),
                                 0x80 | ((code_lo >> 6) & 0x3f),
                                 0x80 | (code_lo & 0x3f))))
        else:
            raise UnicodeEncodeError('mutf-8', input, i, i + 1,
                                     'illegal code point')

    return (res.getvalue(), i)


def decode_mutf8(input, errors='strict'):
    res = io.StringIO()

    num_next = 0

    i = 0
    code = 0
    start = 0

    code_surrogate = None
    start_surrogate = None

    def raise_error(start, reason):
        raise UnicodeDecodeError('mutf-8', input, start, i + 1, reason)

    def check_invalid(cond, start, reason):
        if cond:
            raise_error(start, reason)

    for i, byte in enumerate_bytes(input):
        if (byte & 0x80) == 0x00:
            check_invalid(num_next > 0, start, 'invalid continuation byte')
            num_next = 0
            code = byte
            start = i
        elif (byte & 0xc0) == 0x80:
            check_invalid(num_next < 1, start, 'invalid start byte')
            num_next -= 1
            code = (code << 6) | (byte & 0x3f)
        elif (byte & 0xe0) == 0xc0:
            check_invalid(num_next > 0, start, 'invalid continuation byte')
            num_next = 1
            code = byte & 0x1f
            start = i
        elif (byte & 0xf0) == 0xe0:
            check_invalid(num_next > 0, start, 'invalid continuation byte')
            num_next = 2
            code = byte & 0x0f
            start = i
        else:
            raise_error(i, 'invalid start byte')

        if num_next == 0:
            if code >= 0xd800 and code <= 0xdbff:  # High surrogate
                check_invalid(code_surrogate is not None, start_surrogate,
                              'invalid high surrogate')
                code_surrogate = code
                start_surrogate = start
                continue

            if code >= 0xdc00 and code <= 0xdfff:   # Low surrogate
                check_invalid(code_surrogate is None, start,
                              'invalid low surrogate')
                code = ((code_surrogate & 0x3f) << 10) | (code & 0x3f) + 0x10000
                code_surrogate = None
                start_surrogate = None
            else:
                check_invalid(code_surrogate is not None, start_surrogate,
                              'illegal surrogate')

            res.write(create_chr(code))

    check_invalid(num_next > 0, start, 'unexpected end')
    check_invalid(code_surrogate is not None, start_surrogate, 'unexpected end')

    return (res.getvalue(), i)

def probe_mutf8(name):
    if name == 'mutf-8':
        return codecs.CodecInfo(encode_mutf8, decode_mutf8)
    return None

codecs.register(probe_mutf8)


def create_struct(name, fields):
    field_names = [name for name, ty in fields]
    cls = collections.namedtuple(name, field_names)
    cls.struct_fmt = ''.join(ty for name, ty in fields)
    cls.struct_size = struct.calcsize(cls.struct_fmt)
    def unpack_from(cls, buf, offset=0):
        unpacked = struct.unpack_from(cls.struct_fmt, buf, offset)
        return cls.__new__(cls, *unpacked)
    cls.unpack_from = classmethod(unpack_from)
    return cls


if sys.version_info >= (3,):
    def extract_string(buf, offset=0):
        end = buf.find(b'\0', offset)
        if end == -1:
            return buf[offset:]
        return buf[offset:end].decode('mutf-8')
else:
    def extract_string(buf, offset=0):
        end = buf.find(b'\0', offset)
        if end == -1:
            return buf[offset:]
        return buf[offset:end].decode('mutf-8').encode('utf-8')


def extract_uleb128(buf, offset=0):
    num_bytes = 0
    result = 0
    shift = 0
    while True:
        byte = buf[offset + num_bytes]
        result |= (byte & 0x7f) << shift
        num_bytes += 1
        if (byte & 0x80) == 0:
            break
        shift += 7
    return (result, num_bytes)


def dump_dex_file_buffer(buf):
    Header = create_struct('Header', (
        ('magic', '8s'),
        ('checksum', 'I'),
        ('signature', '20s'),
        ('file_size', 'I'),
        ('header_size', 'I'),
        ('endian_tag', 'I'),
        ('link_size', 'I'),
        ('link_off', 'I'),
        ('map_off', 'I'),
        ('string_ids_size', 'I'),
        ('string_ids_off', 'I'),
        ('type_ids_size', 'I'),
        ('type_ids_off', 'I'),
        ('proto_ids_size', 'I'),
        ('proto_ids_off', 'I'),
        ('field_ids_size', 'I'),
        ('field_ids_off', 'I'),
        ('method_ids_size', 'I'),
        ('method_ids_off', 'I'),
        ('class_defs_size', 'I'),
        ('class_defs_off', 'I'),
        ('data_size', 'I'),
        ('data_off', 'I'),
    ))

    StringId = create_struct('StringId', (
        ('string_data_off', 'I'),
    ))

    header = Header.unpack_from(buf, offset=0)

    offset_start = header.string_ids_off
    offset_end = offset_start + header.string_ids_size * StringId.struct_size
    for offset in range(offset_start, offset_end, StringId.struct_size):
        string_data_offset = StringId.unpack_from(buf, offset).string_data_off
        utf16_len, num_bytes = extract_uleb128(buf, string_data_offset)
        string_data = extract_string(buf, string_data_offset + num_bytes)
        print(utf16_len, repr(string_data))


def dump_dex_file(dex_file):
    dex_file_stat = os.fstat(dex_file.fileno())
    if not dex_file_stat.st_size:
        raise ValueError('empty file')

    with mmap(dex_file.fileno(), dex_file_stat.st_size,
              access=ACCESS_READ) as buf:
        dump_dex_file_buffer(buf)


def generate_classes_dex_names():
    yield 'classes.dex'
    for i in itertools.count(start=2):
        yield 'classes{}.dex'.format(i)


def dump_apk_file(apk_file):
    with zipfile.ZipFile(apk_file, 'r') as zip_file:
        try:
            for name in generate_classes_dex_names():
                with zip_file.open(name) as dex_file:
                    print('### OPEN:', name)
                    dump_dex_file_buffer(get_py3_bytes(dex_file.read()))
        except KeyError:
            pass


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('dex_file')
    return parser.parse_args()


def main():
    args = parse_args()

    with open(args.dex_file, 'rb') as dex_file:
        if args.dex_file.endswith('.dex'):
            dump_dex_file(dex_file)
        else:
            dump_apk_file(dex_file)

if __name__ == '__main__':
    main()

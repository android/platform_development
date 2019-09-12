#!/usr/bin/env python3

"""This module contains utilities to read ELF files."""

import re
import subprocess


_ELF_CLASS = re.compile(
    '\\s*Class:\\s*(.*)$')
_DT_NEEDED = re.compile(
    '\\s*0x[0-9a-fA-F]+\\s+\\(NEEDED\\)\\s+Shared library: \\[(.*)\\]$')
_DT_SONAME = re.compile(
    '\\s*0x[0-9a-fA-F]+\\s+\\(SONAME\\)\\s+Library soname: \\[(.*)\\]$')


def readobj(path):
    """Read ELF bitness, DT_SONAME, and DT_NEEDED."""

    # Read ELF class (32-bit / 64-bit)
    proc = subprocess.Popen(['readelf', '-h', path], stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    stdout = proc.communicate()[0]
    is_32bit = False
    for line in stdout.decode('utf-8').splitlines():
        match = _ELF_CLASS.match(line)
        if match:
            if match.group(1) == 'ELF32':
                is_32bit = True

    # Read DT_SONAME and DT_NEEDED
    proc = subprocess.Popen(['readelf', '-d', path], stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    stdout = proc.communicate()[0]
    dt_soname = None
    dt_needed = set()
    for line in stdout.decode('utf-8').splitlines():
        match = _DT_NEEDED.match(line)
        if match:
            dt_needed.add(match.group(1))
            continue
        match = _DT_SONAME.match(line)
        if match:
            dt_soname = match.group(1)
            continue

    return is_32bit, dt_soname, dt_needed

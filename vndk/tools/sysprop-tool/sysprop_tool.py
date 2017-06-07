#!/usr/bin/env python3

import argparse
import re
import sys

def parse_sysprop(line):
    # Format: `[key]: [value]`
    key, value = line.split(']: [', 1)
    return key[1:], value[:-1]

def build_sysprop_matcher(sysprops):
    sysprops = [b'(?:' + re.escape(p).encode('utf-8') + b')' for p in sysprops]
    return re.compile(b'(?:' + b'|'.join(sysprops) + b')', re.M | re.S)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--system')
    parser.add_argument('--vendor')
    parser.add_argument('--sysprop', required=True)
    args = parser.parse_args()

    with open(args.sysprop, 'r') as f:
        sysprops = [parse_sysprop(line)[0] for line in f.read().splitlines()]

    print(sysprops)

    sysprops_matcher = build_sysprop_matcher(sysprops)

    print(sysprops)

if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python3
from sourcedr.data_utils import load_data
import argparse
import collections
import json
import os

def is_valid(parser, arg):
    if not os.path.exists(arg):
        parser.error('The file {} does not exist!'.format(arg))
    else:
        return arg

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', dest='input_filename', required=True,
                        type=lambda x: is_valid(parser, x))
    parser.add_argument('-o', dest='output_filename', required=True)
    args = parser.parse_args()
    with open(args.input_filename, 'r') as f:
        graph = json.load(f)
    dep = collections.defaultdict(list)
    sos = set()

    for key, value in graph.items():
        if key.split('.')[-1] == 'so':
            sos.add(key)
        for v in value:
            if v.split('.')[-1] == 'so':
                sos.add(v)

    for s in sos:
        # iterative dfs
        visited = []
        stack = [s]
        while stack:
            v = stack.pop()
            if v not in visited:
                visited += [v]
                if v != s:
                    dep[s].append(v)
                try:
                    stack += graph[v]
                except KeyError:
                    pass

    table = collections.defaultdict(list)
    data = load_data()
    for key, item in data.items():
        table[key.split(':')[0]] += item[0]

    res = collections.defaultdict(list)
    for key, items in dep.items():
        try:
            res[key] += table[key]
        except KeyError:
            pass
        for item in items:
            try:
                res[key] += table[item]
            except KeyError:
                pass

    with open(args.output_filename, 'w') as f:
        json.dump(res, f, sort_keys=True, indent=4)

if __name__ == '__main__':
    main()

#!/usr/bin/env python3
from __future__ import print_function

import re
import os
import csv
import subprocess

ALL_PROCESS_HEADER = [
    'pss',
    'proc_name',
    'pid',
]

PROCESS_DETAILS_HEADER = [
    'name',
    'size',
    'rss',
    'pss',
    'swap',
    'swappss (kB)',
]

def write_list_to_csv(path, header, data):
    with open(path, 'w') as csvfile:
        writer = csv.writer(csvfile, delimiter=',')
        writer.writerow(header)
        writer.writerows(data)

def get_process_list():
    """
    get pss & pid of all processes with dumpsys meminfo
    """
    meminfo = subprocess.check_output(
        "adb shell dumpsys meminfo",
        shell=True, universal_newlines=True)

    proc = set()
    pattern = re.compile('\\s*([\\d,]+)K: (.*) \\(pid (\\d+)\\)$')
    for line in meminfo.splitlines():
        match = pattern.match(line)
        if not match:
            continue
        pss = int(match.group(1).replace(',', ''))
        name = match.group(2)
        pid = match.group(3)
        proc.add((pss, name, pid))
    proc = sorted(proc, reverse=True)
    return proc


def get_process_details(proc):
    """
    get process details with smaps
    """
    os.makedirs('details', exist_ok=True)
    lib = {}
    for pid in proc:
        try:
            smaps = subprocess.check_output(
                "adb shell cat /proc/%s/smaps | "
                "grep -e '^........-........ .... ........' "
                "-e '^Size' -e '^Rss' -e '^Pss' -e  '^Swap'" % pid,
                shell=True, universal_newlines=True).splitlines()
        except subprocess.CalledProcessError:
            pass

        data = []
        total = [0] * 5
        for cnt in range(0, len(smaps), 6):
            if len(smaps[cnt].split()) < 6:
                continue
            name = '({}) {}'.format(
                smaps[cnt].split()[1],
                smaps[cnt].split()[5]
            )
            size = smaps[cnt + 1].split()[1]
            rss = smaps[cnt + 2].split()[1]
            pss = smaps[cnt + 3].split()[1]
            swap = smaps[cnt + 4].split()[1]
            swappss = smaps[cnt + 5].split()[1]

            # calculate total memory of rw
            if name not in lib:
                lib[name] = [0] * 5
            for i in range(5):
                total[i] += int(smaps[cnt + i - 5].split()[1])
                lib[name][i] += int(smaps[cnt + i - 5].split()[1])
            data.append([name, size, rss, pss, swap, swappss])
        data.append(['total'] + total)

        write_list_to_csv(
            os.path.join('details', '%s.csv' % pid),
            PROCESS_DETAILS_HEADER, data)
        print('PID %s: done' % pid)

    return lib

def statistic(lib_dir):
    """
    statistic (sys & each lib's total pss of rw )
    """
    lib_arr = []
    total = [0] * 5
    for name in lib_dir:
        mem = lib_dir[name]
        lib_arr.append(tuple([name] + mem))
        for i in range(5):
            total[i] += mem[i]
    lib_arr.sort(key=lambda tup: tup[3], reverse=True)
    lib_arr.append(['total'] + total)

    write_list_to_csv(
        os.path.join('details', 'total.csv'),
        PROCESS_DETAILS_HEADER, lib_arr)

    print('total done')

def main():
    """
    memory analysis
    """
    print('Dumping meminfo ...')
    proc = get_process_list()
    write_list_to_csv('all_process.csv', ALL_PROCESS_HEADER, proc)
    lib = get_process_details([line[2] for line in proc])
    statistic(lib)
    print('Success!')

if __name__ == '__main__':
    main()

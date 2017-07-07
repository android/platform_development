#!/usr/bin/env python3
from __future__ import print_function

import re
import os
import csv
import subprocess

def all_process(to_file=True):
    """
    get pss & pid of all processes with dumpsys meminfo
    """
    meminfo = subprocess.check_output(
        "adb shell dumpsys meminfo | grep pid",
        shell=True, universal_newlines=True)

    proc = []
    for line in meminfo.splitlines():
        pattern = re.compile('\\s*([\\d,]+)K: (.*) \\(pid (\\d+)\\)$')
        match = pattern.match(line)
        if not match:
            continue
        pss = int(match.group(1).replace(',', ''))
        name = match.group(2)
        pid = match.group(3)
        proc.append((pss, name, pid))
    proc = list(set(proc))
    proc.sort(reverse=True)

    if to_file:
        with open('all_process.csv', 'w') as csvfile:
            writer = csv.writer(csvfile, delimiter=',')
            writer.writerow(['pss', 'proc_name', 'pid'])
            writer.writerows(proc)
    return proc


def proc_details(proc):
    """
    get process details with smaps
    """
    if os.path.isdir('details') == False:
        os.mkdir('details')

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

        with open(os.path.join('details', '%s.csv' % pid), 'w') as csvfile:
            writer = csv.writer(csvfile, delimiter=',')
            writer.writerow([
                'name', 'size', 'rss',
                'pss', 'swap', 'swappss (kB)'
            ])

            total = [0] * 5
            for cnt in range(0, len(smaps), 6):
                if len(smaps[cnt].split()) < 6:
                    continue

                name = '({}) {}'.format(
                    smaps[cnt].split()[1],
                    smaps[cnt].split()[5]
                )
                size = smaps[cnt+1].split()[1]
                rss = smaps[cnt+2].split()[1]
                pss = smaps[cnt+3].split()[1]
                swap = smaps[cnt+4].split()[1]
                swappss = smaps[cnt+5].split()[1]

                # calculate total memory of rw
                if name not in lib:
                    lib[name] = [0] * 5
                for i in range(5):
                    total[i] += int(smaps[cnt + i - 5].split()[1])
                    lib[name][i] += int(smaps[cnt + i - 5].split()[1])

                writer.writerow([
                    name, size, rss,
                    pss, swap, swappss
                ])

            writer.writerow(['total'] + total)
        print ('PID %s: done' % pid)
    statistic(lib)

def statistic(lib_dir):
    """
    statistic (sys & each lib's total pss of rw )
    """
    lib_arr = []
    total = [0] * 5

    with open(os.path.join('details', 'total.csv'), 'w') as csvfile:
        writer = csv.writer(csvfile, delimiter=',')
        writer.writerow([
            'name', 'size', 'rss',
            'pss', 'swap', 'swappss (kB)'
        ])
        for name in lib_dir:
            mem = lib_dir[name]
            lib_arr.append(tuple([name] + mem))
            for i in range(5):
                total[i] += mem[i]

        lib_arr.sort(key=lambda tup: tup[3], reverse=True)
        writer.writerows(lib_arr)
        writer.writerow(['total'] + total)

    print ('total done')

def main():
    """
    memory analysis
    """
    print ('Dumping meminfo ...')
    proc = all_process()
    proc_details([line[2] for line in proc])
    print ('Success!')

if __name__ == '__main__':
    main()

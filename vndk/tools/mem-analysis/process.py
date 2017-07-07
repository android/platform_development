import os
import csv
import subprocess
from operator import itemgetter

# get pss & pid of all processes with dumpsys meminfo
def all_process(to_file=True):
    meminfo = subprocess.check_output("adb shell dumpsys meminfo | grep pid ", shell=True, universal_newlines=True)
    proc = []
    for pro in meminfo.split('\n'):
        tmp = pro.split()
        if tmp == []:
            continue
        pss  = tmp[0].replace(',', '').replace(':', '')
        name = tmp[1]
        pid  = tmp[3].replace(')', '')
        proc.append((pss, name, pid))

    proc = list(set(proc))
    proc.sort(key=lambda tup: int(tup[0][:-1]) * -1)

    if to_file:
        with open('all_process.csv', 'w') as fp:
            writer = csv.writer(fp, delimiter=',')
            writer.writerow(['pss', 'proc_name', 'pid'])
            writer.writerows(proc)

    return proc

# get process details with smaps
def proc_details(proc):
    if os.path.isdir('details') == False:
        os.mkdir('details')

    lib = {}

    for pro in proc:
        pid = pro[2]
        cmd = "adb shell cat /proc/%s/smaps | grep -e '^........-........ .... ........' -e '^Size' -e '^Rss' -e '^Pss' -e  '^Swap'" % pid
        try:
            smaps = subprocess.check_output(cmd, shell=True, universal_newlines=True).split('\n')
        except Exception:
            pass

        with open(os.path.join('details', '{}.csv'.format(pid)), 'w') as fp:
            writer = csv.writer(fp, delimiter=',')
            writer.writerow(['name', 'size', 'rss', 'pss', 'swap', 'swappss (kB)'])

            total = [0] * 5
            cnt = 0
            while True:
                tmp = smaps[cnt]
                if tmp == '':
                    break
                tmp = tmp.split()

                name = ''
                if len(tmp) >= 6:
                    name = '(%s) %s' % (tmp[1], tmp[5])
                size    = smaps[cnt+1].split()[1]
                rss     = smaps[cnt+2].split()[1]
                pss     = smaps[cnt+3].split()[1]
                swap    = smaps[cnt+4].split()[1]
                swappss = smaps[cnt+5].split()[1]
                cnt += 6

                # claculate total memery of rw
                if '(rw-p)' in name:
                    if lib.get(name) == None:
                        lib[name] = [0] * 5
                    for i in range(5):
                        total[i] += int(smaps[cnt + i + 1 - 6].split()[1])
                        lib[name][i] += int(smaps[cnt + i + 1 - 6].split()[1])
                writer.writerow([name, size, rss, pss, swap, swappss])

            writer.writerow(['total'] + total)

        print ('PID %s: done' % pid)

    statistic(lib)


# statistic (sys & each lib's total pss of rw )
def statistic(lib_dir):

    lib_arr = []
    total = [0] * 5

    with open (os.path.join('details', 'total.csv'), 'w') as fp:
        writer = csv.writer(fp, delimiter=',')
        writer.writerow(['name', 'size', 'rss', 'pss', 'swap', 'swappss (kB)'])
        for name in lib_dir:
            mem = lib_dir[name]
            lib_arr.append(tuple([name] + mem))
            for i in range(5):
                total[i] += mem[i] 

        lib_arr.sort(key=lambda tup: tup[3] * -1)
        writer.writerows(lib_arr)
        writer.writerow(['total'] + total)

    print ('total done')


def main():
    print ('Dumping meminfo ...')
    proc = all_process()
    proc_details(proc)
    print ('Success!')


if __name__ == '__main__':
    main()

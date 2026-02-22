import os
import sys
import subprocess

info_solver = dict()
def check_all_cec_log_files():
    for root, dirs, files in os.walk(os.path.dirname(os.path.realpath(__file__))):
        for file in files:
            if file.endswith('_cec.log') and file.startswith('replace_'):
                nid = file.split('_')[1]
                info_solver[nid] = dict()
                with open(os.path.join(root, file), 'r') as f:
                    for line in f.readlines():
                        if 'conflicts     :' in line:
                            info_solver[nid]['conflict'] = int(line.split()[2])
                        if 'propagation' in line:
                            info_solver[nid]['propagation'] = int(line.split()[2])
                        if 'decision' in line:
                            info_solver[nid]['decision'] = int(line.split()[2])
                        if 'Final SAT' in line:
                            info_solver[nid]['time'] = float(line.split()[7])
def check_info_file():
    flag = 0
    nid = -1
    with open('_info.log', 'r') as f:
        for line in f.readlines():
            if (flag == 0 and 'UNSAT (rewritable):' in line): flag = 1
            elif (flag == 1):
                if ('costThreshold' in line):
                    l_split = line.split()
                    nid = l_split[0]
                    if (nid not in info_solver): info_solver[nid] = dict()
                    info_solver[nid]['absId'] = int(l_split[1][1:-2])
                    info_solver[nid]['p_conflict'] = int(l_split[10][:-1])
                    # print(line.split())
                elif ('absSize' in line):
                    l_split = line.split()
                    info_solver[nid]['absSize'] = int(l_split[2][:-1])
                    info_solver[nid]['absHeight'] = int(l_split[5][:-1])
                    # print(line.split())
                elif ('patchSize' in line):
                    l_split = line.split()
                    info_solver[nid]['patchSize'] = int(l_split[2][:-1])
                    info_solver[nid]['patchHeight'] = int(l_split[5][:-1])
                    # print(line.split())
check_all_cec_log_files()
check_info_file()
for nid in info_solver:
    if (info_solver[nid]['time'] < 431.86 * 1.1):
        print(nid, end=',')
print('')
for nid in info_solver:
    if (info_solver[nid]['time'] > 431.86 * 0.9):
        print(nid, end=',')
print('')
# for nid in info_solver:
#     print(nid, end=',')
#     for key in info_solver[nid]:
#         print(info_solver[nid][key], end=',')
#     print('')
# print(info_solver)
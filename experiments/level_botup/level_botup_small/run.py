import os
import sys
import subprocess

def make_coef(path, coef):
    with open(os.path.join(path, '_coef.log'), 'w') as f:
        f.write(','.join(map(str, coef)) + '\n')

cur_dir = 0
coef = [-14,1,1,4,2]
while True:
    path = "{}".format(cur_dir)
    make_coef(path, coef)
    cmd = "~/abc_fdrw/abc -c \"&read _init.aig; &fdrw -g -G 5 -a -F _coef.log -C 1500\" > _test.log"
    subprocess.run(cmd, cwd = path, shell=True, check=True)
    path_nxt = "{}".format(cur_dir + 1)
    subprocess.run("mkdir {}".format(path_nxt), shell=True, check=True)
    subprocess.run("cp {}/replaced.aig {}/_init.aig".format(path, path_nxt), shell=True, check=True)
    cur_dir += 1
    if (coef[1] == 1):
        coef[1] = 2
    else:
        coef[1] = 1
        coef[2] += 1
        coef[3] += 1
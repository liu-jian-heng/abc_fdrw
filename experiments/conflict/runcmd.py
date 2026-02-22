import os
import sys
import subprocess

def count_dir_in_path(path):
    dirs = [d for d in os.listdir(path) if os.path.isdir(os.path.join(path, d))]
    count = len(dirs)
    return count

def read_coef(path):
    ''' infoDict should contain:
        ckt: rewrited circuit
        partSyn: method id of partSyn
        Result: the aig of next ite
        C: #conflict
        basicCmd: command'''
    infoDict = dict()
    with open(path + "/info.log", "r") as f:
        for line in f.readlines():
            vals = line.split(':')
            infoDict[vals[0]] = vals[1][:-1]
    return infoDict

def prepare_env(path, infoDict):
    tosynCkt = '2' if infoDict['ckt'] == '1' else '1'
    subprocess.run("mkdir {}".format(path), shell = True)
    subprocess.run("cp {} {}/_init.aig".format(infoDict['Result'], path), shell = True)
    with open(path + "/info.log", "w") as f:
        f.write("ckt:{}\n".format(tosynCkt))
        f.write("partSyn:{}\n".format(infoDict['partSyn']))
        f.write("Result:{}\n".format(path + '/' + infoDict['Result'].split("/")[-1]))
        f.write("C:{}\n".format(infoDict['C']))
        f.write("basicCmd:{}\n".format(infoDict['basicCmd']))
    with open(path + "/_coef.log", "w") as f:
        f.write("-7,{},{}\n".format(tosynCkt, infoDict['partSyn']))
def get_cmd(infoDict):
    return "~/abc_fdrw/abc -c \"&read _init.aig; &fdrw {} -F _coef.log -C {}\" > _result.log".format(infoDict['basicCmd'], infoDict['C'])
def run_cmd(path, cmd):
    subprocess.run(cmd, cwd = path, shell = True)
    if (os.path.exists(path + "/partSyn.aig")):
        subprocess.run("~/abc_fdrw/abc -c \"&read partSyn.aig; &fdrw -S partSyn.stat -N -2\" > sweep.log", cwd = path, shell = True)
        # subprocess.run("rm partSyn.aig", cwd = path, shell = True)
        subprocess.run("mv syn.aig partSyn.aig", cwd = path, shell = True)
        subprocess.run("mv syn.stat partSyn.stat", cwd = path, shell = True)

def check_result(path):
    with open(path + "/_result.log", "r") as f:
        for line in f.readlines():
            if ("conflict test failed" in line): return 0
            if ("PO patch is found" in line): return -1
            if ("no patch is found" in line): return 0
    assert( "final.aig" in os.listdir(path) )
    return 1

if (len(sys.argv) > 2):
    path = sys.argv[1]
    dirs = [d for d in os.listdir(path) if os.path.isdir(os.path.join(path, d))]
    if (sys.argv[2] == 'rm'):
        for dir in dirs:
            if dir != '0': subprocess.run("rm -rf {}".format(path + '/' + dir), shell = True)
    if (sys.argv[2] == 'rm1'):
        prev_dir = str(count_dir_in_path(path) - 1)
        subprocess.run("rm -rf {}".format(path + '/' + prev_dir), shell = True)

else:
    flag = 1
    while (True):
        path = sys.argv[1]
        if (flag == 1):
            prev_dir = str(count_dir_in_path(path) - 1)
            dir_name = str(count_dir_in_path(path))
            infoDict = read_coef(path + "/" + prev_dir)
            print(infoDict)
            # subprocess.run("mkdir {}/{}".format(path, dir_name), shell = True)
            prepare_env(path + "/" + dir_name, infoDict)
            infoDict = read_coef(path + "/" + dir_name)
            cmd = get_cmd(infoDict)
            print(cmd)
            run_cmd(path + "/" + dir_name, cmd)
            flag = check_result(path + "/" + dir_name)
        elif (flag == 0):
            infoDict['C'] = round(int(infoDict['C']) * 1.2)
            print('C is updated to {}'.format(infoDict['C']))
            if (infoDict['C'] > 9999): break
            cmd = get_cmd(infoDict)
            print(cmd)
            run_cmd(path + "/" + dir_name, cmd)
            flag = check_result(path + "/" + dir_name)
        elif (flag == -1):
            break
import os
import gc
import time
import math
import signal
import random
import argparse
import subprocess
import multiprocessing
from utils import run_clam
from utils import process_bugs
from utils import process_clam
from utils import process_files
from subprocess import Popen, PIPE, STDOUT

#COLORS------------------------
TGREEN =  '\033[32m'
TRED =  '\033[31m'
TWHITE = '\033[37m'
TYELLOW = '\033[33m'
TBLUE = '\033[34m'
TPURPLE = '\033[35m'
TTIRQUOISE = '\033[36m'
TDEFAULT = '\033[0m'

bugs = []

def exit_handler(signum, frame):
    print('Process Terminated....')
    exit(0)


def timeout_handler(signum, frame):
    raise TimeoutError("This file took too long")


def get_processes():
    os.system("ps -A --no-headers | wc -l")
    #print(f"{TRED}{p}{TDEFAULT}")


def create_new_mount_folder(core):
    path_to_mount = process_files.get_value_json(config, "mount_folder") + f"/core_{core}"
    fol = process_files.find_folders(path_to_mount)

    if fol == []:
        #in this is the first file we add
        working_folder = "file_0"   
    else:
        prev_index = max(list(int(f.split("_")[1]) for f in fol))
        working_folder = "file_"+ str(int(prev_index) + 1)
    
    #print(f"to create folder {working_folder}")
    os.system(f"mkdir {path_to_mount}/{working_folder}")
    return f"{path_to_mount}/{working_folder}"


def get_size(string):
    " returns the first number found in a string like"
    "224K ../c/whnoc/cwnjcne.c"
    s = string.split()[0]
    num = ""
    unit = 0.0
    for char in s:
        if char.isdigit() or char == ".":
            num = num + char
        elif char == "B":
            unit = 1.0
        elif char == "K":
            unit = 1000.0
        elif char == "M":
            unit = 1000000.0
        else:
            print("could not calculate size")
            return 0

    num = int(float(num) * unit)
    return num


def randomize_params(core, inv_folder):
    config = inv_folder + "/config.json"
    #options: ------------------------------------------
    domains = ["w-int","int", "soct", "zones"]
    ctracks = ["sing-mem","num", "mem"]
    analysis = ["inter", "intra", "backward"]  #inter and backward CAN NOT be both TRUE
    inline = [True, False]  
    widening_delay = [1,2,4,8,16]
    narrowing_iterations = [1,2,3,4]
    widening_jump_set = [10,20,30,40]
    percentage = [40, 50, 60, 70, 80, 90, 95]
    
    process_files.update_json(config, "domain", random.choice(domains))
    process_files.update_json(config, "ctrack", random.choice(ctracks))
    process_files.update_json(config, "inline", random.choice(inline))
    process_files.update_json(config, "analysis", random.choice(analysis))
    process_files.update_json(config, "percentage", random.choice(percentage))
    process_files.update_json(config, "widening_delay", random.choice(widening_delay))
    process_files.update_json(config, "widening_jump_set", random.choice(widening_jump_set))
    process_files.update_json(config, "narrowing_iterations", random.choice(narrowing_iterations))


def run(core, inv_folder):
    config = inv_folder + "/config.json"

    sleep = process_files.get_value_json(config, "sleep")
    ctrack = process_files.get_value_json(config, "ctrack")
    domain = process_files.get_value_json(config, "domain")
    file_name = process_files.get_value_json(config, "file")
    directory = process_files.get_value_json(config, "directory")

    inline = int(process_files.get_value_json(config, "inline")) * "--inline"
    
    analysis = process_files.get_value_json(config, "analysis")
    an_crab = ""
    if analysis == "inter":
        an_crab = "--crab-inter"
    elif an_crab == "backward":
        an_crab = "--crab-backward"

    #ensure that directory and file_name have correct format
    if directory != "":
        if directory[-1] != "/" :
            directory = directory + "/"   

    if file_name[-2:] == ".c":
        file_name = file_name[:-2]
 
    #delete previous folders in inv_folder and create new ones
    folders = process_files.find_folders(inv_folder)
    for fol in folders:
        process_files.delete_all_but_config(f"{inv_folder}/{fol}")
    
    process_files.delete_non_c_files(inv_folder)
    
    #####################################################
    #if files are destined for seahorn !!
    #run clam to delete all but 1 assertions
    #####################################################

    #Call clam to produce .bc and transformed files  
    ret = run_clam.produce_transformed_files(config, inv_folder)
    if ret == False:    #case of early termination due to error or timeout
        print(f"{directory}{file_name}\t core {core}\t {TYELLOW} Timeout, Memory Out or Other Error{TDEFAULT} ")
        return False

    #check that initial.bc is produced:
    if not process_files.find_file(f"{file_name}.bc", inv_folder):
        print(f"{directory}{file_name}\t core {core}\t {TYELLOW} No .bc file was produced from clam{TDEFAULT} ")
        return False

    if not process_files.find_file(f"initial.bc", inv_folder):
        print(f"{directory}{file_name}\t core {core}\t {TYELLOW} initial.bc file was produced from clam{TDEFAULT} ")
        return False

    #Call clam to find initial warnings in inv_folder file.bc
    #path_to_file = f"{inv_folder}/{file_name}.bc"
    path_initial_file = f"{inv_folder}/initial.bc"

    #find new errors ------------------------------------------------------------------------------
    for folder in folders:
        f = process_files.get_bc_files(f"{inv_folder}/{folder}")
        
        path_transformed_file = f"{inv_folder}/{folder}/{f}"
        transformation_mode = folder.split("_")[0]
        inner_inv_folder = f"{inv_folder}/{folder}"
        
        if f == [] :
            print(f"{directory}{file_name}\t core {core}\t {TYELLOW} No transformed .bc file was found in {folder} {TDEFAULT} ")
            continue
        elif len(f)>1:
            continue

        f = f[0]

        if file_name not in f:
            print(f"{directory}{file_name}\t core {core}\t {TTIRQUOISE} File {file_name} does not match transformed {f} {TDEFAULT} ")
            return False

        mount_folder = create_new_mount_folder(core)

        process_files.update_json(config, "tr_mode", transformation_mode)
        os.system(f"cp {inv_folder}/{folder}/log.txt {mount_folder}/log.txt") #!!ensure it exists
        os.system(f"cp {config} {mount_folder}/config.json")
        os.system(f"cp {path_initial_file} {mount_folder}/initial.bc")
        os.system(f"cp {path_transformed_file} {mount_folder}/transformed.bc")
        os.system(f"cp {directory}{file_name}.c {mount_folder}/{file_name}.c")

    time.sleep(sleep)
    return True



def main(core, inv_folder):
    
    signal.signal(signal.SIGALRM, timeout_handler)
    
    config = inv_folder + "/config.json"
    dd = process_files.get_value_json(config, "program_folder")

    max_file_size = process_files.get_value_json(config, "max_file_size")
    
    folders_to_ignore = []  #["ldv-multiproperty", "sqlite", "eca-programs", "eca-rers2012", "eca-rers2018"]
    files_to_ignore = bugs
    folders = process_files.find_folders(dd)
    random.shuffle(folders)

    while(True):

        for folder in folders:
            if folder in folders_to_ignore:
                continue

            #get all files in the folder
            path = dd + "/" + folder
            files = process_files.get_c_files(path)

            for f in files:

                if f in files_to_ignore:
                    print(f"{path}{f}\t core {core}\t  {TPURPLE}File has bugs{TDEFAULT}")
                    continue

                gc.collect()

                size_info = os.popen(f"du -sh {path}/{f}")
                output = size_info.readlines()[0]
                size = get_size(output)
                
                if size > max_file_size:
                    #print(f"{path}{f}\t core {core}\t  {TPURPLE}File was of size {size}B, which was above the limit{TDEFAULT}")
                    continue
                
                #update config.json with new parameters
                process_files.update_json(config, "file", f)
                process_files.update_json(config, "directory", path)
                
                randomize_params(core, inv_folder)
                run(core, inv_folder)           
 


if __name__ == "__main__":
    os.system("rm -r invariants")
    os.system("mkdir invariants")

    signal.signal(signal.SIGTERM, exit_handler)
    
    cores = int(process_files.get_value_json("config.json", "cores"))
    dd = process_files.get_value_json("config.json", "program_folder")
    reft = int(process_files.get_value_json("config.json", "run_each_file_times")) 

    path = "/clam/build"
    os.system(f"cmake --build {path} --target install")             #build pass with output
    
    processes = ""
    process_pid = []

    #find bug files, so that they are not tested again
    bug_folders = process_files.find_folders("bugs")
    for fol in bug_folders:
        path = "bugs" + "/" + fol
        config = path + "/config.json"
        bug_file = process_files.get_value_json(config, "file")

        if bug_file not in bugs:
            print(f"new file bug in : {path}")
            bugs.append(bug_file)

    print(bugs)

    try:        
        for core in range(cores):           
            process_files.create_new_inv_folder(core)

            for i in range(reft):
                if i < math.floor(reft / 2) :
                    os.popen(f"mkdir invariants/core_{core}/stronger_{i}")
                else :
                    os.popen(f"mkdir invariants/core_{core}/weaker_{i}")
            
            process = multiprocessing.Process(target=main, args=(core, f"invariants/core_{core}"))
            process.start()
            processes = processes + str(process.pid) + " "
            process_pid.append(str(process.pid))           
            # pin the process to a specific CPU        
            os.system("taskset -p -c " + str(core) + " " + str(process.pid))

        while True:
            continue

    except (KeyboardInterrupt, SystemExit):
        
        for i in process_pid:
            os.kill(int(i), signal.SIGTERM)
        
        time.sleep(1)
        print("\nGood Bye!\n")



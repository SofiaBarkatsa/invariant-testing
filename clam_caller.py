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
    assertion_percentage = [50] #range(10, 40, 5)
    
    #domain used for analysis
    process_files.update_json(config, "domain", random.choice(domains))
    process_files.update_json(config, "ctrack", random.choice(ctracks))
    process_files.update_json(config, "inline", random.choice(inline))
    process_files.update_json(config, "analysis", random.choice(analysis))
    process_files.update_json(config, "percentage", random.choice(percentage))
    process_files.update_json(config, "widening_delay", random.choice(widening_delay))
    process_files.update_json(config, "widening_jump_set", random.choice(widening_jump_set))
    process_files.update_json(config, "narrowing_iterations", random.choice(narrowing_iterations))
    process_files.update_json(config, "assertion_percentage", random.choice(assertion_percentage))

    if process_files.get_value_json(config, "different_domains") == True:
        domains.remove(process_files.get_value_json(config, "domain"))
        process_files.update_json(config, "assumption_domain", random.choice(domains))
    else:
        domain = process_files.get_value_json(config, "domain")
        process_files.update_json(config, "assumption_domain", domain)


def run(core, inv_folder):
    config = inv_folder + "/config.json"

    sleep = process_files.get_value_json(config, "sleep")
    ctrack = process_files.get_value_json(config, "ctrack")
    domain = process_files.get_value_json(config, "domain")
    file_name = process_files.get_value_json(config, "file")
    directory = process_files.get_value_json(config, "directory")
    ass_domain = process_files.get_value_json(config, "assumption_domain")

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
    path_to_file = f"{inv_folder}/initial.bc"
    warnings, ass1 = run_clam.run_and_find_warnings(config, inv_folder, path_to_file)
    #print(warnings, ass1)

    #find new errors ------------------------------------------------------------------------------
    for folder in folders:
        f = process_files.get_bc_files(f"{inv_folder}/{folder}")
        
        if f == [] :
            print(f"{directory}{file_name}\t core {core}\t {TYELLOW} No transformed .bc file was found in {folder} {TDEFAULT} ")
            continue
        elif len(f)>1:
            continue

        f = f[0]

        if file_name not in f:
            print(f"{directory}{file_name}\t core {core}\t {TTIRQUOISE} File {file_name} does not match transformed {f} {TDEFAULT} ")
            return False

        #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        path_to_file = f"{inv_folder}/{folder}/{f}"
        inner_inv_folder = f"{inv_folder}/{folder}"
        warnings2, ass2 = run_clam.run_and_find_warnings(config, inner_inv_folder, path_to_file)
        #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        #check for correct results
        if warnings == -1 :
            print(f"{directory}{file_name}\t core {core}\t {TYELLOW} Clam failed to analyze initial .bc file {TDEFAULT} ")
            return False
       
        if warnings == -2 :
            print(f"{directory}{file_name}\t core {core}\t {TYELLOW} Timeout or Memory on Initial.bc {TDEFAULT} ")
            return False

        if warnings2 == -1 :
            print(f"{directory}{file_name}\t core {core}\t {TYELLOW} Clam failed to analyze transformed.bc file {TDEFAULT} ")
            return False

        if warnings2 == -2 :
            print(f"{directory}{file_name}\t core {core}\t {TYELLOW} Timeout or Memory on Transformed.bc {TDEFAULT} ")
            return False

        
        #########################################
        result = process_clam.check_assertions(inv_folder, inner_inv_folder)
        if result == -1:
            print(f"{directory}{file_name}\t core {core}\t {TYELLOW} Error with assertions {TDEFAULT} ")
            return

        #metamorphic testing: comparison 
        transformation_mode = folder.split("_")[0]

        failed = (transformation_mode == "stronger" and warnings < warnings2) or \
            (transformation_mode == "weaker" and not (warnings == warnings2) and (ass_domain==domain)) or \
            (transformation_mode == "weaker" and  (warnings > warnings2) and (ass_domain!=domain)) 
            #or \
            #result == 1 or result >= 3
        
        store = failed
        #rejected = ass1 != ass2
        rejected = (result == 2)

        print_str = f"{directory}{file_name}  core {core}  "
        print_str = print_str + f"{TPURPLE}({ass_domain} -> {domain})  "
        #print_str = print_str + f"{TPURPLE}({domain},  {ctrack},  {analysis})  "
        print_str = print_str + f"{TBLUE}{transformation_mode}  "
        print_str = print_str + f"{TDEFAULT * (not failed)}{TRED * failed}{warnings} -> {warnings2}  "
        #print_str = print_str + f"{TDEFAULT * (not rejected)}{TYELLOW * rejected}({ass1} -> {ass2})  "
        print_str = print_str + rejected * f"{TYELLOW}Rejected (assertion missmatching)" + \
                    (failed and not rejected)* f"{TRED}Fail" + \
                    (not failed and not rejected) * f"{TGREEN}Pass" + TDEFAULT
        print_str = print_str + (failed and not rejected and result == 1) * f"{TRED} (assertion Failure)" + \
                    (failed and not rejected and result >= 3) * f"{TRED} (assertion Failure & missmatch)" + \
                    TDEFAULT
        print_str = print_str + f"({ass1}->{ass2})"
        print(print_str)
        
        if store: # and not rejected:
            bugs.append(f"{file_name}.c")
            process_bugs.add_files(file_name, directory, inv_folder, f"{inv_folder}/{folder}")    ##TODO

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
    domains = ["int", "soct", "zones"]
    ctracks = ["sing-mem","num", "mem"]

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
            
            
            process_files.update_json(f"invariants/core_{core}/config.json", "domain", domains[core % len(domains)])
            process_files.update_json(f"invariants/core_{core}/config.json", "ctrack", ctracks[core % len(ctracks)])
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



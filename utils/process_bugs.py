import os
import json
import sys
from utils import process_files

# setting path
sys.path.append('/clam')
from clam_debugger import minimize
from clam_debugger import debug

working_folder = ""
TGREEN =  '\033[32m'
TRED =  '\033[31m'
TDEFAULT = '\033[0m'

def create_new_bug_folder():
    fol = process_files.find_folders("/clam/bugs")
    global working_folder

    if fol == []:
        #in this is the first bug we find
        working_folder = "bug_0"   
    else:
        prev_index = max(list(int(f.split("_")[1]) for f in fol))
        working_folder = "bug_"+ str(int(prev_index) + 1)
    
    #print(f"to create folder {working_folder}")
    os.system(f"mkdir bugs/{working_folder}")


def add_files(file, directory, inv_folder, subfolder):
    #file = test not test.c
    #file must NOT have a type: i.e test1 and not test1.c    
    create_new_bug_folder()

    if process_files.find_file("log.txt", subfolder):
        os.system(f"cp {subfolder}/log.txt bugs/{working_folder}/transformed_log.txt")

    if process_files.find_file("assertionsLOG.txt", subfolder):
        os.system(f"cp {subfolder}/assertionsLOG.txt bugs/{working_folder}/assertionsLOG.txt")
    
    if process_files.find_file("log.txt", inv_folder):
        os.system(f"cp {inv_folder}/log.txt bugs/{working_folder}/initial_log.txt")

    #copy c file
    os.system(f"cp {directory}/{file}.c bugs/{working_folder}/{file}.c")
    
    #copy config file
    os.system(f"cp {inv_folder}/config.json bugs/{working_folder}/config.json") 
    
    #copy initial .bc file
    os.system(f"cp {inv_folder}/initial.bc bugs/{working_folder}/initial.bc")
    os.system(f"llvm-dis bugs/{working_folder}/initial.bc")
    
    #copy transformed .bc file
    tr = process_files.get_bc_files(subfolder)
    if tr != []:
        os.system(f"cp {subfolder}/{tr[0]} bugs/{working_folder}/{tr[0]}")
        os.system(f"llvm-dis bugs/{working_folder}/{tr[0]}")
    
    #copy .crabir files
    f = process_files.get_crabir_files(subfolder) 
    for i in f:
        os.system(f"cp {subfolder}/{i} bugs/{working_folder}/{i}")

    f = process_files.get_crabir_files(inv_folder) 
    for i in f:
        os.system(f"cp {inv_folder}/{i} bugs/{working_folder}/{i}")


    #check that everything works correctly, or remove folder
    #check_passed = check_that_assumptions_cause_bug()
    #if check_passed:
    #    check_if_reproducable()


def check_that_assumptions_cause_bug():
    #check using delta debugging that at least 1 assumption is responsible
    #for the wrong number of warnigns
    assumptions = len(minimize(f"bugs/{working_folder}"))
    if assumptions == 0:
        num = working_folder.split("_")[1]
        print(f"{TRED} Bug {num} was not caused by adding assumptions \
            \n Deleting bug folder ... {TDEFAULT}")
        os.system(f"rm -rf bugs/{working_folder}")
        return False
    else:
        print(f"{TGREEN} Bug was caused by {assumptions} assumptions {TDEFAULT}")
        return True

def check_if_reproducable():
    all_good = debug(f"bugs/{working_folder}")
    if all_good:
        #if all good, then no bug was detected
        num = working_folder.split("_")[1]
        print(f"{TRED} Bug {num} could not be reproduced \n Deleting bug folder ... {TDEFAULT}")
        os.system(f"rm -rf bugs/{working_folder}")
    else:
        print(f"{TGREEN} Bug could be reproduced {TDEFAULT}")

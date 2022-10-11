import os
import json
from utils import process_files

#check assertions: return codes
# -1 : Something went wrong, i.e. files were not found -> terminate
#  0 : All assertions passed, no issue there
#  1 : There where failures -> i.e. Assertions went from passing to failing
#  2 : Assertions were missing 
#  3 : Assertions were missing & there were failures

def get_number(string):
    " returns the first number found in a string"
    numbers = [int(s) for s in string.split() if s.isdigit()]
    return int(numbers[0])

def look_for_analysis_results(output):
    if ("************** ANALYSIS RESULTS ****************\n" in output) or \
        ("************** ANALYSIS RESULTS ****************" in output):
        return True
    else:
        return False


def find_warnings(output):
    "Function that takes as input the output from clam.py analysis "
    "This function returns the number of warnings produced by clam"
    warnings = -1
    errors = -1
    safe = -1 
    #in case of read_results.py
    checked = -1
    unchecked = -1 
    #total number of assertions found
    total = -1
    inter = False
    failed = -1
    for row in range(len(output)):
        if (output[row] == "************** ANALYSIS RESULTS ****************\n") or \
            (output[row] == "************** ANALYSIS RESULTS ****************"):
            warnings = get_number(output[row+3])
            errors = get_number(output[row+2])
            safe = get_number(output[row+1])
            failed = warnings + errors
        elif "-- Failed assertions   :" in output[row]:
            inter = True
            failed = get_number(output[row])
        elif "-- Checked assertions  :" in output[row]:
            """print(output[row])
            print(output[row+1])
            print(output[row+2])
            print(output[row+3])"""
            inter = True
            checked = get_number(output[row])
        elif "-- Unchecked assertions" in output[row]:
            inter = True
            unchecked = get_number(output[row])

    num_of_assertions = inter * (checked + unchecked) + (not inter) * (warnings + errors + safe)
    return failed, num_of_assertions


def check_assertions(inv_folder, tr_folder):
    tr_mode = tr_folder.split("/")[-1].split("_")[0]
    init = process_files.get_crabir_files(inv_folder)
    tran = process_files.get_crabir_files(tr_folder)
    init.sort()
    tran.sort()
    
    f = open(f"{tr_folder}/assertionsLOG.txt", "a")
    f.write("Logging assertions...\n")
    
    if len(init) != len(tran):
        return -1  #not enough files
    if len(init) == 0 or len(init) == None:
        return -1  #no files

    result = 0
    if len(init) == 1:
        #file names like filename.crabir
        current_init = init[0]
        current_tran = tran[0]
        result = compare_assertions(f"{inv_folder}/{current_init}",
             f"{tr_folder}/{current_tran}", tr_mode, f)
    else:
        #file names like filename.functionName.crabir
        init_functions = list(i.split(".")[-2] for i in init).sort()
        tran_functions = list(i.split(".")[-2] for i in tran).sort()
        if init_functions != tran_functions:
            return -1

        res = 0
        for i in range(len(init)):
            current_init = init[i]
            current_tran = tran[i]
            res = compare_assertions(f"{inv_folder}/{current_init}",
                f"{tr_folder}/{current_tran}", tr_mode, f)
            f.write("\n ---------------------------------- \n")
            #print(f"RESULT = {res}")
            result = result + (result != res) * res

    f.close()
    return result


def compare_assertions(path_to_init, path_to_tran, tr_mode, f):
    init = []
    tran = []
    #-------logging---------------------------------
    f.write("Comparing assertions in files:\n")
    f.write(f"{path_to_init} \n")
    f.write(f"{path_to_tran} \n \n")

    with open(path_to_init) as file:
        init = file.readlines()
    with open(path_to_tran) as file:
        tran = file.readlines()

    #find assertions and save them in the dictionary
    init_dict = {}
    tran_dict = {}
    for i in range(len(init)):
        line = init[i]
        if ("// loc(file" in line) :
            if ("id=" in line) and ("Result" in line):
                Id, res = find_id_result(line)
                init_dict[Id] = res
            else:
                print("error in line", line)
    
    for i in range(len(tran)):
        line = tran[i]
        if ("// loc(file" in line) :
            if ("id=" in line) and ("Result" in line):
                Id, res = find_id_result(line)
                tran_dict[Id] = res
            else:
                print("error in line", line)

    #f.write(f"initial dict : \n {init_dict} \n")
    #f.write(f"tran dict : \n {tran_dict} \n")

    #metamorphic testing:
    # 0) if the dicts do not have the same size, there is
    # an assertion missing error
    # 1) 
    # if transformations are weaker, then initial and 
    # transformed dictionaries NEED to be the same.
    # 2) 
    # if transformations are stronger, then initial assertions
    # that passed can NOT have turned into assertions that fail
    if len(init_dict.keys()) != len(tran_dict.keys()) and tr_mode == "weaker":
        f.write("ASSERTION MISSING\n")
        set1 = set(init_dict.items())
        set2 = set(tran_dict.items())
        f.write("initial file had:\n")
        f.write(f"{set1 - set2}\n")
        f.write("transformed had:\n")
        f.write(f"{set2 - set1}\n")
        return 2

    passed = True

    if tr_mode == "weaker":
        if init_dict == tran_dict:
            f.write("PASS \n")
            return 0
        else:
            f.write("FAIL \n")
            f.write(f"initial dict was:\n {init_dict}\n")
            f.write(f"transformed dict had instead:\n ")
            set1 = set(init_dict.items())
            set2 = set(tran_dict.items())
            f.write(f"{set2 - set1}")
            passed = False
            return 1
    else:
        if init_dict == tran_dict:
            f.write("PASS \n")
            return 0
        else:
            for key in init_dict.keys():
                if init_dict[key] == True and key in tran_dict.keys():
                    if tran_dict[key] == False:
                        f.write("FAIL \n")
                        f.write(f"initially:\n id = {key} -> {init_dict[key]}\n")
                        f.write(f"transformed:\n id = {key} -> {tran_dict[key]}\n")
                        passed = False
            if passed:
                f.write("PASS \n")
                return 0
            
            return 1



def find_id_result(line):
    #// loc(file=none line=-1 col=-1) id=1 Result:  FAIL -- num of warnings=1
    a = line.split(" ")
    res = -1
    Id = -1
    for i in range(len(a)):
        if "id" in a[i]:
            Id = int(a[i].split("=")[1])
        if "Result" in a[i]:
            res = False * ("FAIL" in a[i+2]) + True * ("OK" in a[i+2])
    #print(f"in line {line} id= {Id} and result= {res}")
    return Id, res

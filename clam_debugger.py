import clam_caller
import argparse
import os
import math
from utils import process_files
from utils import delta_debugger
from utils import run_clam

TGREEN =  '\033[32m'
TRED =  '\033[31m'
TWHITE = '\033[37m'
TYELLOW = '\033[33m'
TBLUE = '\033[34m'
TPURPLE = '\033[35m'
TTIRQUOISE = '\033[36m'
TDEFAULT = '\033[0m'


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


def rename_bugs(args):
    # okey
    bug_folders = process_files.find_folders(args.d)
    mx = 0
    for fol in bug_folders:
        mx = max(mx, int(fol.split("_")[1]))

    for f in range(len(bug_folders)):
        fol = bug_folders[f]
        name = "bug_" + str(mx + f + 1)
        os.system(f"mv bugs/{fol} bugs/{name}")

    bug_folders = process_files.find_folders(args.d)
    for f in range(len(bug_folders)):
        fol = bug_folders[f]
        name = "bug_" + str(f)
        os.system(f"mv bugs/{fol} bugs/{name}")


def test_file(args):
    # okey
    os.system("rm -r invariants")
    os.system("mkdir invariants")
    os.system("mkdir invariants/core_0")
    os.system(f"cp {args.config} invariants/core_0/config.json")

    reft = int(process_files.get_value_json(args.config, "run_each_file_times")) 
    process_files.update_json(args.config, "core", 0)
    
    for i in range(reft):
        if i < math.floor(reft / 2) :
            os.popen(f"mkdir invariants/core_0/stronger_{i}")
        else :
            os.popen(f"mkdir invariants/core_0/weaker_{i}")
    
    clam_caller.run(0, "invariants/core_0")


def narrow_down_bugs(args):
    # works but could be improved
    bug_folders = process_files.find_folders(args.d)
    bug_files = []

    for fol in bug_folders:
        path = args.d + "/" + fol
        config = path + "/config.json"

        bug_file = process_files.get_value_json(config, "directory") + "/"
        bug_file = bug_file + process_files.get_value_json(config, "file")

        analysis = process_files.get_value_json(config, "analysis")
        domain = process_files.get_value_json(config, "domain")
        #domain = process_files.get_value_json(config, "domain")
        
        #widening_delay = process_files.get_value_json(config, "widening_delay")
        #widening_jump_set = process_files.get_value_json(config, "widening_jump_set")
        #narrowing_iterations = process_files.get_value_json(config, "narrowing_iterations")

        tpl = (bug_file, analysis, domain)

        ll_files = process_files.get_ll_files(path)
        if "minimized.ll" in ll_files:
            ll_files.remove("minimized.ll")

        if tpl not in bug_files:
            print(f"new file bug in : {path} \t  {tpl}")
            bug_files.append(tpl)
        else:
            os.system(f"rm -rf {path}")

    print("\n\n The remaining files are: ")
    for f in bug_files:
        print(f)



def debug(args):
    #probably ok
    #args = bug_folder
    config = args + "/config.json"
    
    domain = process_files.get_value_json(config, "domain")
    file_name = process_files.get_value_json(config, "file")
    directory = process_files.get_value_json(config, "directory")
    analysis = process_files.get_value_json(config, "analysis")
    ctrack = process_files.get_value_json(config, "ctrack")

    bc_files = process_files.get_bc_files(args)
    ll_files = process_files.get_ll_files(args)
    if "minimized.ll" in process_files.get_ll_files(args):
        ll_files.remove("minimized.ll")
    #Call clam to find warnings ----------------------------------------------------------
    #warnings on initial.bc
    path_to_file=""

    if "initial.ll" in ll_files:
        path_to_file = f"{args}/initial.ll"
        ll_files.remove("initial.ll")
    else:
        path_to_file = f"{args}/initial.bc"
    path_to_file = f"{args}/initial.bc"
    warnings, ass1 = run_clam.run_and_find_warnings(config, args, path_to_file)

    #-------------------------------------------------------------------------------------------
    #warnings on transformed.bc
    path_to_file2 = f"{args}/{ll_files[0]}"
    warnings2, ass2 = run_clam.run_and_find_warnings(config, args, path_to_file2)
    #-------------------------minimizer------------------------------------------------------
    warnings3 = -1
    ass3 = -1
    if "minimized.ll" in process_files.get_ll_files(args):
        path_to_file3 = f"{args}/minimized.ll"
        warnings3, ass3 = run_clam.run_and_find_warnings(config, args, path_to_file3)
 
    transformation_mode = ""
    for f in bc_files:
        if "stronger" in f:
            transformation_mode = "stronger"
        elif "weaker" in f:
            transformation_mode = "weaker"

    extra_tab=""
    extra_warnings = ""
    if transformation_mode == "weaker":
        extra_tab = "\t"
    if warnings3 != -1:
        extra_warnings = f" min:{warnings3} "

    failed = (transformation_mode == "stronger" and warnings < warnings2) or \
            (transformation_mode == "weaker" and not (warnings == warnings2)) 
    store = failed
    rejected = ass1 != ass2

    print_str = f"{directory}{file_name}  "
    print_str = print_str + f"{TPURPLE}({domain},  {ctrack},  {analysis})  "
    print_str = print_str + f"{TBLUE}{transformation_mode}  "
    print_str = print_str + f"{TDEFAULT * (not failed)}{TRED * failed}{warnings} -> {warnings2}  "
    print_str = print_str + f"{TDEFAULT * (not rejected)}{TYELLOW * rejected}({ass1} -> {ass2})  "
    print_str = print_str + rejected * f"{TYELLOW}Rejected" + \
                (failed and not rejected)* f"{TRED}Fail" + \
                (not failed and not rejected) * f"{TGREEN}Pass" + TDEFAULT

    print(print_str)
    if failed:
        return False
    else :
        return True 




def minimize(bug_folder):
    files = process_files.get_ll_files(bug_folder)
    if "minimized.ll" in files:
        files.remove("minimized.ll")
    transformed_file = max(files)
    
    d = delta_debugger.DeltaDebugger(transformed_file, bug_folder)
    assumptions = d.ddmin()

    return assumptions
    #if minimized file exists: llvm pass DCE


def debug_all(args):
    bug_folders = process_files.find_folders(args.d)
    for fol in bug_folders:
        no_bug = debug(f"{args.d}/{fol}")
        if no_bug:
            os.system(f"rm -rf {args.d}/{fol}")


def minimize_all(args):
    bug_folders = process_files.find_folders(args.d)
    many_assumptions = []
    for fol in bug_folders:
        assumptions = minimize(f"{args.d}/{fol}")
        
        print("------------------------------------------------------------------------------")
        debug(f"{args.d}/{fol}")
        print(f"{fol} was caused by {TRED}{len(assumptions)} {TDEFAULT}assumptions")
        for i in assumptions:
            print(i)

        if len(assumptions) == 0:
            os.system(f"rm -rf {args.d}/{fol}")
        elif len(assumptions)>1:
            many_assumptions.append(fol)

    print("bugs caused by more than one assumpton:")
    print(many_assumptions)


def test_all(args):
    bug_folders = process_files.find_folders(args.d)
    for fol in bug_folders:
        args.config = args.d + "/" + fol + "/config.json"
        test_file(args)


def update_json_files(args):
    bug_folders = process_files.find_folders(args.d)
    for fol in bug_folders:
        path = args.d + "/" + fol
        config = path + "/config.json"

        if process_files.get_value_json(config, "ctrack") ==None:
            process_files.update_json(config, "ctrack", "num")
        if process_files.get_value_json(config, "inline") ==None:
            process_files.update_json(config, "inline", True)
        if process_files.get_value_json(config, "analysis") ==None:
            process_files.update_json(config, "analysis", "inter")
        if process_files.get_value_json(config, "widening_delay") ==None:
            process_files.update_json(config, "widening_delay", 1)
        if process_files.get_value_json(config, "narrowing_iterations") ==None:
            process_files.update_json(config, "narrowing_iterations", 10)
        if process_files.get_value_json(config, "widening_jump_set") ==None:
            process_files.update_json(config, "widening_jump_set", 0)
        if process_files.get_value_json(config, "alarm") ==None:
            process_files.update_json(config, "alarm", 4)
        if process_files.get_value_json(config, "mem") ==None:
            process_files.update_json(config, "mem", 500)


def find_smallest_file(args, ignore_folders):
    bug_folders = process_files.find_folders(args.d)
    smallest_file = ""
    min_size = 1000000000000
    
    for fol in bug_folders:
        if fol not in ignore_folders:
            size_info = os.popen(f"du -sh {args.d}/{fol}/initial.ll")
            output = size_info.readlines()[0]
            size = get_size(output)

            if size < min_size :
                smallest_file = fol
                min_size = size
            
    return [smallest_file, min_size]


def find_smallest_files(args):
    num_of_files = 2
    folders = []
    for i in range(num_of_files):
        ans = find_smallest_file(args, folders)
        folders.append(ans[0])
        config = args.d + "/" + ans[0] + "/config.json"
        print_str = f"In {ans[0]} file of size {ans[1]} \t (" 
        print_str += process_files.get_value_json(config, "analysis") + ", domain:"
        print_str += process_files.get_value_json(config, "domain") + ")"
        print(print_str)



if __name__ == "__main__":
    path = "/clam/build"
    os.system(f"cmake --build {path} --target install > /dev/null")             #build pass with output

    p = argparse.ArgumentParser()

    # files----------------------------------------------------------------------
    # 1 of these 3 must be defined
    file_group = p.add_mutually_exclusive_group()
    

    file_group.add_argument("-f", help="A .c file that will be used for testing")
    file_group.add_argument("-d", help="A directory with .c files to be used for testing")
    file_group.add_argument("-dd", help="A directory of directories with .c files for testing")
    file_group.add_argument("-config", help="A config.json file that will be used for testing")


    # options -------------------------------------------------------------------
    
    p.add_argument("--narrow-down-bugs", action = 'store_true', default = False , \
        help="Keeps 1 bug from each file")
    
    p.add_argument("--debug", action = 'store_true', default = False , \
        help="given a bugs/bug_X folder, reruns clam to see Pass/Fail")

    p.add_argument("--debug-all", action = 'store_true', default = False , \
        help="given a bugs/bug_X folder, reruns clam to see Pass/Fail")
    
    p.add_argument("--rename-bugs", action = 'store_true', default = False , \
        help="rename bug folders")

    p.add_argument("--minimize-all", action = 'store_true', default = False , \
        help="Minimize all bug transformed.ll files")
    
    p.add_argument("--test-all", action = 'store_true', default = False , \
        help="Minimize all bug transformed.ll files")

    p.add_argument("--update-json-files", action = 'store_true', default = False , \
        help="Updates all json files, so that they have all the newest parameters")

    p.add_argument("--find-smallest-files", action = 'store_true', default = False , \
        help="Updates all json files, so that they have all the newest parameters")

    args = p.parse_args()

    if (args.config):
        test_file(args)

    if (args.d):
        if (args.update_json_files):
            update_json_files(args)

        if (args.debug):
            debug(args.d)

        if(args.debug_all):
            debug_all(args)

        if (args.narrow_down_bugs):
            narrow_down_bugs(args)
            
        if(args.rename_bugs):
            rename_bugs(args)

        if(args.minimize_all):
            minimize_all(args)

        if (args.test_all):
            test_all(args)

        if (args.find_smallest_files):
            find_smallest_files(args)

        

    #print(run_clam.run_and_find_warnings("invariants/core_0/config.json", " ", "invariants/core_0/stronger_0/barrier_2t_stronger_0.ll"))

import os
import io
import gc
import sys
import json
import time
import math
import signal
import random
import argparse
from .process_files import get_files
from .process_files import get_value_json
from .process_clam import find_warnings
from .process_clam import look_for_analysis_results

def remove_ending(path_to_file):
    if path_to_file[-2:] == ".c":
        path_to_file = path_to_file[:-2]
    elif path_to_file[-3:] == ".ll" or path_to_file[-3:] == ".bc":
        path_to_file = path_to_file[:-3]
    return path_to_file


def produce_transformed_files(config, inv_folder):
    #calls clam.py with additional parameters, using the Optimizer pass
    #in order to produce N=reft files which have been transformed


    mem =  get_value_json(config, "mem")
    seed =  get_value_json(config, "seed")
    sleep =  get_value_json(config, "sleep")
    alarm =  get_value_json(config, "alarm")
    ctrack =  get_value_json(config, "ctrack")
    file_name =  get_value_json(config, "file")
    project =  get_value_json(config, "project")
    analysis =  get_value_json(config, "analysis") 
    directory =  get_value_json(config, "directory")
    # percentage: how many assumptions will transform
    percentage =  get_value_json(config, "percentage")
    domain = get_value_json(config, "assumption_domain")
    reft =  get_value_json(config, "run_each_file_times")
    widening_delay = get_value_json(config, "widening_delay")
    enable_warnings =  get_value_json(config, "enable-warnings")
    widening_jump_set =  get_value_json(config, "widening_jump_set")
    narrowing_iterations =  get_value_json(config, "narrowing_iterations")
    assertion_percentage =  get_value_json(config, "assertion_percentage")
    oracle_assertions =  int(get_value_json(config, "oracle_assertions"))* "--oracle-assertions"
    random_assertions =  int(get_value_json(config, "random_assertions"))* "--random-assertions"
    stronger_assertions =  int(get_value_json(config, "stronger_assertions"))* "--stronger-assertions"
    # stronger assertions will be added based on "percentage", unless Stronger_assertions=false
    
    inline = int( get_value_json(config, "inline")) * "--inline"

    for_project=""
    if project=="smack":
        for_project = "--smack"

    if directory != "":
        if directory[-1] != "/" :
            directory = directory + "/"   

    if file_name[-2:] == ".c":
        file_name = file_name[:-2]

    os.system(f"clang -S -emit-llvm -o {inv_folder}/{file_name}.ll {directory}{file_name}.c ")    

    proc = os.system(f"clam.py --domain={domain} --no-preprocess --crab-disable-warnings --crab-promote-assume\
        --crab-opt=add-invariants --crab-opt-invariants-loc=block-entry \
            --percentage={percentage} --inv-folder={inv_folder} --num-of-files={reft}\
            --c-widening-delay={widening_delay} --c-narrowing-iterations={narrowing_iterations} --c-widening-jump-set={widening_jump_set} \
                --c-track={ctrack}  {inline}  --c-analysis={analysis}  \
                    {random_assertions} {oracle_assertions} {stronger_assertions}  \
                    --assertion-percentage={assertion_percentage}  --crab-disable-warnings\
                    -o {inv_folder}/{file_name}.bc --cpu={alarm} --mem={mem} {for_project} \
                    {inv_folder}/{file_name}.ll {enable_warnings} > /dev/null ")  # 2>/dev/null 

    
    if proc != 0:
        print("\n#################################################\n To see the error, type these commands\n")
        print(f"clang -S -emit-llvm -o {inv_folder}/{file_name}.ll {directory}{file_name}.c ")
        print("\n")
        print(f"clam.py --domain={domain} --no-preprocess --crab-disable-warnings --crab-promote-assume --crab-opt=add-invariants --crab-opt-invariants-loc=block-entry  --percentage={percentage} --inv-folder={inv_folder} --num-of-files={reft} --c-widening-delay={widening_delay} --c-narrowing-iterations={narrowing_iterations} --c-widening-jump-set={widening_jump_set}  --c-track={ctrack}  {inline}  --c-analysis={analysis} --assertion-percentage={assertion_percentage} {oracle_assertions} {stronger_assertions} {random_assertions} --crab-disable-warnings  -o {inv_folder}/{file_name}.bc --cpu={alarm} --mem={mem} {for_project} {inv_folder}/{file_name}.ll")
        print("#################################################\n")
    
    return proc == 0 



def run_and_find_warnings(config, inv_folder, path_to_file):
    #based on config file, this function finds warnings of the file given
    #this file can end in .c, .bc or .ll, but the .bc file is the one that actually gets executed

    mem =  get_value_json(config, "mem")
    seed =  get_value_json(config, "seed")
    sleep =  get_value_json(config, "sleep")
    alarm =  get_value_json(config, "alarm")
    ctrack =  get_value_json(config, "ctrack")
    domain =  get_value_json(config, "domain")
    file_name =  get_value_json(config, "file")
    analysis =  get_value_json(config, "analysis") 
    directory =  get_value_json(config, "directory")
    percentage =  get_value_json(config, "percentage")
    reft =  get_value_json(config, "run_each_file_times")
    widening_delay =  get_value_json(config, "widening_delay")
    enable_warnings =  get_value_json(config, "enable-warnings")
    widening_jump_set =  get_value_json(config, "widening_jump_set")
    narrowing_iterations =  get_value_json(config, "narrowing_iterations")

    inline = int(get_value_json(config, "inline")) * "--inline"
    
    analysis = get_value_json(config, "analysis")
    an_crab = ""
    if analysis == "inter":
        an_crab = "--crab-inter"
    elif analysis == "backward":
        an_crab = "--crab-backward"

    if directory != "":
        if directory[-1] != "/" :
            directory = directory + "/"   

    if path_to_file[-2:] == ".c":
        path_to_file2 = path_to_file[:-2]
    elif path_to_file[-3:] == ".bc":
        path_to_file2 = path_to_file[:-3]
    elif path_to_file[-3:] == ".ll":
        path_to_file2 = path_to_file[:-3]

    ocrab = f"-ocrab={path_to_file2}.crabir" #* (analysis=="inter")

    stream = os.popen(f"clam.py  --crab-disable-warnings --no-preprocess --crab-check=assert --crab-dom={domain} {inline} \
        --crab-disable-warnings --crab-track={ctrack} --crab-lower-unsigned-icmp=true {an_crab} \
        --crab-widening-delay={widening_delay} --crab-narrowing-iterations={narrowing_iterations} --crab-widening-jump-set={widening_jump_set} \
            --cpu={alarm} --mem={mem} {path_to_file} {ocrab} {enable_warnings}")  # 2>/dev/null
    
    output = stream.readlines()
    
    if look_for_analysis_results(output) == False:
        #case of error----------
        for i in output:
            print(i)
        if len(output)<1:
            print("Sofia: no output")
        return -2, 0
    
    if analysis == "inter":
        stream = os.popen(f"read_results.py {path_to_file2}.crabir {enable_warnings}")  # 2>/dev/null 
        output = stream.readlines()
    
    """for i in output:
        print(i)"""

    #print(f"""clam.py  --crab-disable-warnings --no-preprocess --crab-check=assert --crab-dom={domain} {inline}  --crab-disable-warnings --crab-track={ctrack} --crab-lower-unsigned-icmp=true {an_crab} --crab-widening-delay={widening_delay} --crab-narrowing-iterations={narrowing_iterations} --crab-widening-jump-set={widening_jump_set} --cpu={alarm} --mem={mem} {path_to_file} {ocrab} {enable_warnings}""")
    
    warnings, num_of_assertions = find_warnings(output)
    #print("file",path_to_file,"warn/assert", warnings, num_of_assertions)
    return warnings, num_of_assertions


def print_run_command(config, inv_folder, path_to_file):
    mem =  get_value_json(config, "mem")
    seed =  get_value_json(config, "seed")
    sleep =  get_value_json(config, "sleep")
    alarm =  get_value_json(config, "alarm")
    ctrack =  get_value_json(config, "ctrack")
    domain =  get_value_json(config, "domain")
    file_name =  get_value_json(config, "file")
    analysis =  get_value_json(config, "analysis") 
    directory =  get_value_json(config, "directory")
    percentage =  get_value_json(config, "percentage")
    reft =  get_value_json(config, "run_each_file_times")
    widening_delay =  get_value_json(config, "widening_delay")
    enable_warnings =  get_value_json(config, "enable-warnings")
    widening_jump_set =  get_value_json(config, "widening_jump_set")
    narrowing_iterations =  get_value_json(config, "narrowing_iterations")

    inline = int(get_value_json(config, "inline")) * "--inline"
    
    analysis = get_value_json(config, "analysis")

    an_crab=""
    if analysis == "inter":
        an_crab = "--crab-inter"
    elif analysis == "backward":
        an_crab = "--crab-backward"

    print(an_crab)
    if directory != "":
        if directory[-1] != "/" :
            directory = directory + "/"   

    if path_to_file[-2:] == ".c":
        path_to_file2 = path_to_file[:-2]
    elif path_to_file[-3:] == ".bc":
        path_to_file2 = path_to_file[:-3]
    elif path_to_file[-3:] == ".ll":
        path_to_file2 = path_to_file[:-3]

    print(f"""clam.py  --crab-disable-warnings --no-preprocess --crab-check=assert --crab-dom={domain} {inline}  --crab-disable-warnings --crab-track={ctrack} --crab-lower-unsigned-icmp=true {an_crab} --crab-widening-delay={widening_delay} --crab-narrowing-iterations={narrowing_iterations} --crab-widening-jump-set={widening_jump_set} --cpu={alarm} --mem={mem} {path_to_file}""")
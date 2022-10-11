import os
import json
from os import listdir

def get_files(path):
    stream = os.popen(f"ls {path}")
    stream = stream.readlines()

    files = list(f[0:-1] for f in stream)
    return files


def get_c_files(path):
    stream = os.popen(f"ls {path}")
    stream = stream.readlines()
    files = list(f[0:-1] for f in stream if f[-3:-1]==".c")
    return files


def get_ll_files(path):
    stream = os.popen(f"ls {path}")
    stream = stream.readlines()
    #might crush for files like a.c
    files = list(f[0:-1] for f in stream if f[-4:-1]==".ll")
    return files


def get_bc_files(path):
    stream = os.popen(f"ls {path}")
    stream = stream.readlines()
    #might crush for files like a.c
    files = list(f[0:-1] for f in stream if f[-4:-1]==".bc")
    return files

def get_crabir_files(path):
    stream = os.popen(f"ls {path}")
    stream = stream.readlines()
    stream = list(s for s in stream if len(s)>8)
    files = list(f[0:-1] for f in stream if f[-8:-1]==".crabir")
    return files

"""
def get_crabir_files(path):
    stream = os.popen(f"ls {path}")
    stream = stream.readlines()
    #might crush for files like a.c
    files = list(f[0:-1] for f in stream if f[-8:-1]==".crabir")
    return files
"""
#-----------------------------------------------------------------------------------
def delete_non_c_files(path):
    ll_files = get_ll_files(path)
    bc_files = get_bc_files(path)
    crab_files = get_crabir_files(path)
    to_delete = ll_files + bc_files + crab_files

    for i in to_delete:
        os.system(f"rm {path}/{i}")


def delete_all_but_config(path):
    files = get_files(path)
    to_delete = list(f for f in files if f != "config.json")

    for i in to_delete:
         os.system(f"rm {path}/{i}")

#----------------------------------------------------------------------------------
def create_folder(name, path):
    folders = find_folders(path)
    if name not in folders:
        os.system(f"mkdir {name}")


def create_new_inv_folder(core):
    """Creation of a new core_X folder inside of Invarians
        and addition of an updated config.json file"""
    prev_index = 0
    working_folder=""

    fol = find_folders("/clam/invariants")
    
    if fol == []:
        #in this is the first core we use
        working_folder = "core_0"   
    else:
        prev_index = max(list(int(f.split("_")[1]) for f in fol))
        working_folder = "core_"+ str(int(prev_index) + 1)
        
        if prev_index + 1 != core:
            print(f"ERROR: invariant folder for core {core} already exists")
            print(f"WARNING: folder 'core_{prev_index}' will be used for core {core}")
            return
                      
    
    os.system(f"mkdir invariants/{working_folder}")

    #create a new updated config file
    os.system(f"cp config.json  ./invariants/{working_folder}/config.json")
    update_json(f"./invariants/{working_folder}/config.json", "core", core)
    update_json(f"./invariants/{working_folder}/config.json", "invariants_folder",\
         f"./invariants/{working_folder}/")



#-----------------------------------------------------------------------------------
def find_folders(path):
    return [name for name in os.listdir(path) if os.path.isdir(f"{path}/{name}")]


def find_ll_file(file, path):
    files = get_ll_files(path)
    if file in files:
        return True
    else :
        return False


def find_file(File, path):
    #print(f"looking for file {file} in {path}")
    files = get_files(path)
    #print(f"files found in path {path} are {files}")
    #print(f"looking for file {File}")
    if files == []:
        return False

    if File in files:
        return True
    else :
        return False

#-----------------------------------------------------------------------------------

def update_json(file, key, value):
    with open(file, "r") as json_file:
        data = json.load(json_file)

    data[key] = value

    with open(file, "w") as outfile:
        json.dump(data, outfile, indent = len(data))


def get_value_json(file, key):
    data = {}
    with open(file, "r") as json_file:
        data = json.load(json_file)

    if key not in data:
        return None
    else :
        return data[key]


# Metamorphic testing of static analysis using inferred invariant transformations
This branch supports llvm14

# Installation:

```
git clone https://github.com/SofiaBarkatsa/invariant-testing.git
```
This project uses [Crab](https://github.com/seahorn/crab) library for program analysis
and [LLVM](https://github.com/llvm/llvm-project) version 14

For this project [rapidjson](https://github.com/Tencent/rapidjson) is also required
and can be install using the command:
```
sudo apt-get install -y rapidjson-dev
```

### In order to build CLAM

#### Requirements
Clam is written in C++ and uses heavily the Boost library. The main requirements are:

- Modern C++ compiler supporting c++14
- Boost >= 1.65
- GMP
- MPFR (if -DCRAB_USE_APRON=ON or -DCRAB_USE_ELINA=ON)

In linux, you can install requirements typing the commands:
```
 sudo apt-get install libboost-all-dev libboost-program-options-dev
 sudo apt-get install libgmp-dev
 sudo apt-get install libmpfr-dev	
```
Also the following python packages are required for testing
```
 pip3 install lit
 pip3 install OutputCheck
```

#### Compiling from sources and installation
Go to clam folder
```
cd clam
```
The compilation steps are:
```
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=_DIR_ ../
cmake --build . --target extra            
cmake --build . --target crab && cmake ..
cmake --build . --target llvm && cmake ..           
cmake --build . --target install 
```

### Changes made in Clam:
- clam_caller.py                      (new)
- clam_debugger.py                    (new)
- <b>utils</b>                        (new folder)
- tools/clam/clam.cc                  (modified)
- lib/Clam/Optimizer/Optimizer.cc     (modified)
- lib/Transforms/DeleteAssume.cc      (new)
- lib/Transforms/DeleteCrabCommand.cc (new)

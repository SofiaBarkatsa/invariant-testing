import io
import os
import copy
from utils import process_clam
from utils import process_files
from utils import run_clam

PASS = 'PASS'
FAIL = 'FAIL'
UNRESOLVED = 'UNRESOLVED'


class DeltaDebugger:

    llFile = ""     #file with assumptions
    testingFile = ""
    bugFolder = ""
    config = ""
    assumptionString = "call void @verifier.assume("
    warnings = 0
    assumptions = []


    def __init__(self, llFile, bugFolder):
        self.llFile = llFile
        self.testingFile = bugFolder + "/minimized.ll"
        self.bugFolder = bugFolder
        self.config = bugFolder+"/config.json"
        self.find_assumptions()
        #print("found assumptions")
        self.warnings= self.find_warnings(f"{bugFolder}/{llFile}")
        #print("found initial warnigns = ", self.warnings)


    def find_warnings(self, path_to_file):
        config = self.config
        inv_folder = self.bugFolder

        """ctrack = process_files.get_value_json(config, "ctrack")
        domain = process_files.get_value_json(config, "domain")

        #Call clam to find warnings ----------------------------------------------------------
        stream = os.popen(f"clam.py --crab-check=assert --crab-dom={domain} \
            --crab-disable-warnings --crab-track={ctrack} --crab-lower-unsigned-icmp --crab-inter \
                {file} ")  # 2>/dev/null
        
        output = stream.readlines()
        new_warnings = process_clam.find_warnings(output)"""

        new_warnings = run_clam.run_and_find_warnings(config, inv_folder, path_to_file)

        return new_warnings



    def find_assumptions(self):
        with open(f"{self.bugFolder}/{self.llFile}") as file:
            po = file.readlines()

        for line in po:
            if self.assumptionString in line:
                self.assumptions.append(line)

        #print(self.assumptions)


    def addSpecificAssumptions(self, list):
        with open(f"{self.bugFolder}/{self.llFile}") as file:
            po = file.readlines()

        for line in po:
            if (self.assumptionString in line) and not (line in list):
                po.remove(line)

        with open(self.testingFile, 'w') as f:
            for line in po:
                f.write(line)


    def mystery(self, inp) :
        
        self.addSpecificAssumptions(inp)
        new_warnings = self.find_warnings(self.testingFile)
       
        if new_warnings == self.warnings:
            raise ValueError("Invalid input")
        else:
            pass


    def ddmin(self) :
        """Reduce the input inp, using the outcome of test(fun, inp)."""
        inp = copy.deepcopy(self.assumptions)
        assert self.test(inp) != PASS

        n = 2     # Initial granularity
        while len(inp) >= 2:
            start = 0
            subset_length = int(len(inp) / n)
            some_complement_is_failing = False

            while start < len(inp):
                complement = (inp[:int(start)] + inp[int(start + subset_length):])      #the substring we analyze

                if self.test(complement) == FAIL:
                    inp = complement
                    n = max(n - 1, 2)
                    some_complement_is_failing = True
                    break

                start += subset_length

            if not some_complement_is_failing:
                if n == len(inp):
                    break
                n = min(n * 2, len(inp))


        self.addSpecificAssumptions(inp)
        return inp


    def test(self, inp, expected_exc = None) :
        result = None
        detail = ""
        try:
            result = self.mystery(inp)
            outcome = PASS
        except Exception as exc:
            detail = f" ({type(exc).__name__}: {str(exc)})"
            if expected_exc is None:
                outcome = FAIL
            elif type(exc) == type(expected_exc) and str(exc) == str(expected_exc):
                outcome = FAIL
            else:
                outcome = UNRESOLVED

        #print(f"mystery({repr(inp)}): {outcome}{detail} \n\n")
        return outcome


#print(ddmin(test, [1,2,3,4,5,6,7]))

if __name__ == '__main__':
    d = DeltaDebugger("test_weaker_4.ll", "bugs/bug_0")
    print(d.ddmin())



#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Pass.h"

#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::PatternMatch;

namespace clam {

/// Returns true if v is used by assume
static bool hasAssumeUsers(Value &v) {
  for (User *U : v.users())
    if (CallInst *ci = dyn_cast<CallInst>(U))
      if (match(ci, m_Intrinsic<Intrinsic::assume>()))
        return true;

  return false;
}

class DeleteAssume : public FunctionPass {
public:
  static char ID;

  DeleteAssume() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) {
     
    if (F.empty())
      return false;

    bool Changed = false;

    LLVMContext &ctx = F.getContext();
    IRBuilder<> Builder(ctx);

    std::vector<Instruction *> to_remove;

    for (auto &I : llvm::make_range(inst_begin(F), inst_end(F))) {
      if (!isa<CallInst>(&I))
        continue;

      CallSite CS(&I);
      const Function *fn = CS.getCalledFunction();
      if (!fn && CS.getCalledValue()) {
        fn = dyn_cast<const Function>(CS.getCalledValue()->stripPointerCasts());
      }

      if (fn && (fn->getName().equals("verifier.assume") || fn->getName().equals("verifier.assume.not"))) {

        Changed = true;
        Value *arg = CS.getArgument(0);

        // already used in llvm.assume.
        if (hasAssumeUsers(*arg)) {
          to_remove.push_back(CS.getInstruction());
          continue;
        }

        /*
          enqueue verifier.assume to be removed
        */
        to_remove.push_back(&I);
      }
    }
    
    
    while (!to_remove.empty()) {
      Instruction *I = to_remove.back();
      to_remove.pop_back();
      I->eraseFromParent();
    }
    
    return Changed;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const {
    // AU.setPreservesAll();
  }

  virtual StringRef getPassName() const {
    return "Clam: Delete verifier.assume instances";
  }
};

char DeleteAssume::ID = 0;

FunctionPass *createDeleteAssumePass() { return new DeleteAssume(); }

} // namespace clam

static llvm::RegisterPass<clam::DeleteAssume>
    X("delete-assume", "Delete verifier.assume instances");

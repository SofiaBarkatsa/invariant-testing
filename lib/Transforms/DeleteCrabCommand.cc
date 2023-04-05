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

class DeleteCrabCommand : public FunctionPass {
public:
  static char ID;

  DeleteCrabCommand() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) {
     
    if (F.empty())
      return false;

    bool Changed = false;

    LLVMContext &ctx = F.getContext();
    IRBuilder<> Builder(ctx);

    std::vector<Instruction *> to_remove;

    for (auto &I : llvm::make_range(inst_begin(F), inst_end(F))) {
      
      if (!(isa<ICmpInst>(&I) || I.isBinaryOp()))
        continue;

      if(I.getValueName()){ 
        if (I.getValueName()->getValue()->getName().startswith("crab")){ 
          //llvm::errs()<< I.getValueName()->getValue()->getName()<<"\n\n";
          to_remove.push_back(&I);
        }
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
    return "Clam: Delete crab produced commands";
  }
};

char DeleteCrabCommand::ID = 0;

FunctionPass *createDeleteCrabCommandPass() { return new DeleteCrabCommand(); }

} // namespace clam

static llvm::RegisterPass<clam::DeleteCrabCommand>
    X("delete-crab-command", "Deletes commands added by the optimizer pass");

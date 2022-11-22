#pragma once

#include "clam/config.h"
#include "llvm/Pass.h"


#include "clam/Clam.hh"

#include "seadsa/InitializePasses.hh"
#include "seadsa/support/RemovePtrToInt.hh"

#include "seadsa/AllocWrapInfo.hh"
#include "seadsa/DsaLibFuncInfo.hh"
#include "clam/CfgBuilder.hh"
#include "clam/HeapAbstraction.hh"
#include "clam/SeaDsaHeapAbstraction.hh"
#include "clam/Support/NameValues.hh"

#include <string> 

namespace clam {
// Preprocessor passes
llvm::Pass *createInsertEntryPointPass();  
llvm::Pass *createLowerCstExprPass();
llvm::Pass *createLowerSelectPass();
llvm::Pass *createLowerUnsignedICmpPass();
llvm::Pass *createMarkInternalInlinePass();
llvm::Pass *createRemoveUnreachableBlocksPass();
llvm::Pass *createSimplifyAssumePass();
llvm::Pass *createDevirtualizeFunctionsPass();
llvm::Pass *createExternalizeAddressTakenFunctionsPass();
llvm::Pass *createExternalizeFunctionsPass();
llvm::Pass *createPromoteMallocPass();
llvm::Pass *createPromoteAssumePass();
llvm::Pass *createRenameNondetPass();
llvm::Pass *createNondetInitPass();
llvm::Pass *createDeadNondetElimPass();
llvm::Pass *createLoopPeelerPass(unsigned Num);
// Visualization passes
llvm::Pass *createAnnotatedCFGPrinterPass();
// Property instrumentation passes
llvm::Pass *createNullCheckPass();
llvm::Pass *createUseAfterFreeCheckPass();
// Postprocessing passes
llvm::Pass *createOptimizerPass(ClamGlobalAnalysis*, int, int, std::string, std::string);  //sofia changed this
llvm::Pass *createDeleteAssumePass();   //sofia added this
llvm::Pass *createDeleteCrabCommandPass();   //sofia added this
} // namespace clam

#ifdef HAVE_LLVM_SEAHORN
#include "llvm_seahorn/Transforms/InstCombine/SeaInstCombine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"

llvm::FunctionPass *
createSeaInstructionCombiningPass(bool ExpensiveCombines,
                                  unsigned MaxIterations, bool AvoidBv,
                                  bool AvoidUnsignedICmp, bool AvoidIntToPtr,
                                  bool AvoidAliasing, bool AvoidDisequalities);

namespace clam {
inline llvm::FunctionPass *createInstCombine(bool ExpensiveCombines = true) {
  const unsigned MaxIterations = 1000; /*same value used by LLVM*/
  const bool AvoidBv = true;
  const bool AvoidUnsignedICmp = true;
  const bool AvoidIntToPtr = true;
  const bool AvoidAliasing = true;
  const bool AvoidDisequalities = true;
  return createSeaInstructionCombiningPass(
      ExpensiveCombines, MaxIterations, AvoidBv, AvoidUnsignedICmp,
      AvoidIntToPtr, AvoidAliasing, AvoidDisequalities);
}
} // namespace clam
#else
namespace clam {
inline llvm::FunctionPass *createInstCombine(bool ExpensiveCombines = true) {
  return llvm::createInstructionCombiningPass(ExpensiveCombines);
}
} // namespace clam
#endif

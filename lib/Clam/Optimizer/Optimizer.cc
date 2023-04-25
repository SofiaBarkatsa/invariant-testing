#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"

#include "clam/config.h"
#include "clam/CfgBuilder.hh"
#include "clam/Clam.hh"
#include "clam/Transforms/Optimizer.hh"
#include "crab/analysis/abs_transformer.hpp"
#include "crab/numbers/wrapint.hpp"

#include "crab/config.h"
#include "crab/support/debug.hpp"

#include <type_traits>
#include <stdlib.h>  
#include <string> 
#include <fstream>
#include <random>

using namespace llvm;

/* Begin LLVM pass options */


static cl::opt<clam::InvariantsLocation>
InvLoc("crab-opt-add-invariants",
    cl::desc("Instrument code with (linear) invariants at specific location"),
    cl::values(clEnumValN(clam::InvariantsLocation::NONE,
			  "none", "Do not add invariants"),
	       clEnumValN(clam::InvariantsLocation::BLOCK,
			  "block-entry",
			  "Add invariants at the entry of each basic block"),
	       clEnumValN(clam::InvariantsLocation::LOOP_HEADER,
			  "loop-header",
			  "Add invariants only at loop headers"),
	       clEnumValN(clam::InvariantsLocation::LOAD_INST,
			  "after-load",
			  "Add invariants after each load instruction"),
	       clEnumValN(clam::InvariantsLocation::ALL,
			  "all",
			  "Add invariants at all locations (very verbose)")),
       cl::init(clam::InvariantsLocation::NONE));

llvm::cl::opt<bool>
RemoveDeadCode("crab-opt-dce",
	 llvm::cl::desc("DCE using Crab invariants"),
	 llvm::cl::init(true));

llvm::cl::opt<bool>
ReplaceWithConstants("crab-opt-replace-with-constants",
	 llvm::cl::desc("Replace values with constants inferred by Crab"),
	 llvm::cl::init(false));


// PARAMETERS BY SOFIA ----------------------------------------------------------------------------
static llvm::cl::opt<int>
    Percentage("percentage",
             llvm::cl::desc("Percentage for Transformations"),
             llvm::cl::init(50));

static llvm::cl::opt<int>
    AssertionPercentage("assertion_percentage",
             llvm::cl::desc("Percentage for Adding Assertions"),
             llvm::cl::init(0));

static llvm::cl::opt<bool>
    OracleAssertions("oracle_assertions",
             llvm::cl::desc("For adding all invariants as Assertions"),
             llvm::cl::init(false));    

static llvm::cl::opt<bool>
    Smack("smack",
             llvm::cl::desc("Assumptions will have attribute noinline as well (llvm12)"),
             llvm::cl::init(false));  
        
//-------------------------------------------------------------------------------------------------

/* End LLVM pass options */

#define DEBUG_TYPE "crab-opt"

STATISTIC(NumDeadBlocks, "Number of dead blocks");
STATISTIC(NumDeadEdges, "Number of dead edges");
STATISTIC(NumInstrBlocks, "Number of blocks instrumented with invariants");
STATISTIC(NumInstrLoads, "Number of load inst instrumented with invariants");


// GLOBAL VARS BY SOFIA------------------------------------------------------------------
std::ofstream out;			//file to write module pass output
int Seed = 0;           // if seed == -1 then no transformations
int AddAssertions = 0;  //if AddAssertions = 1, only then add oracle/random assertions
std::string Mode = "stronger";
std::string Subfolder;
llvm::Function *m_assertFn;
//---------------------------------------------------------------------------------------


// FUNCTIONS BY SOFIA---------------------------------------------------------------------
bool flip_a_coin(int Percentage){
  int num = rand()%100;

  if (num < Percentage) return true;
  else return false;
}

int throw_dice(int options){
  if (options <= 0){
    return 0;
  }
  return rand()%options;
}

int random_normal(int mean_value, int max_value){
  std::random_device mch;
  std::default_random_engine generator(mch());
  std::normal_distribution<double> distribution(0.0, 5.0);
  
  int number = std::ceil(distribution(generator));
  if (number < max_value && number > mean_value){
    return number;
  }
  else {
    while (!(number < max_value && number > mean_value) ){
      number = std::ceil(distribution(generator));
    }
    return number;
  }
}


void write_to_file(clam::lin_cst_t cst) {
  auto e = cst.expression();
  for (auto t : e) {
    clam::number_t n = t.first; 
    int64_t num = int64_t(n); //works
    clam::varname_t v = t.second.name();
    
    if (n == 0)
      continue;
    if (n > 0) {    // && t != cst.variables().begin()
      out << " + ";
    }
    if (n == -1) {
      out << " - ";
    } 
    else if (n != 1) {
      out << num << "*"; 
    }
    out << v.str(); 
  }

  if (cst.is_inequality()) {
    out<<" <= "; 
  } 
  else if (cst.is_equality()) {
     out<<" = "; 
  } 
  // not equal
  else {
     out<<" != "; 
  }

  clam::number_t c = cst.expression().constant();  
  int64_t cnum = int64_t(c); //works
  out << -1* cnum ;//??-1
}


void log_info(llvm::BasicBlock *B, clam::lin_cst_t cst){
  out<<"-------------------------------------------------------------------------------------\n";
  out<<"\nIn function : "<<std::string(B->getParent()->getName())<<"\t In Block "<< std::string(B->getName())<<" :\n";
  out<< "    Initial constraint :   ";
  write_to_file(cst);  
  out<<"\n\n";
  // cst.dump();
} 

//----------------------------------------------------------------------------------------


namespace {

using namespace clam;  
using namespace crab::cfg;


static bool readMemory(const llvm::BasicBlock &B) {
  return std::any_of(B.begin(), B.end(),
		     [](const Instruction &I) {
		       return isa<LoadInst>(I);
		     });
}
static bool hasUnreachable(const llvm::BasicBlock &B) {
  return std::any_of(B.begin(), B.end(),
		     [](const Instruction &I) {
		       return isa<UnreachableInst>(I);
		     });
}
  
static bool requireDominatorTree(InvariantsLocation val) {
  return (val == InvariantsLocation::BLOCK ||
	  val == InvariantsLocation::LOOP_HEADER ||
	  val == InvariantsLocation::ALL);
}

static bool requireLoopInfo(InvariantsLocation val) {
  return val == InvariantsLocation::LOOP_HEADER;
}



// Convert a Crab expression into LLVM bitcode
class CodeExpander {
private:  
  enum bin_op_t { ADD, SUB, MUL };

  static Value *mkBinOp(bin_op_t Op, IRBuilder<> B, Value *LHS, Value *RHS,
		 const Twine &Name) {
    assert(LHS->getType()->isIntegerTy() && RHS->getType()->isIntegerTy());
    switch (Op) {
    case ADD:
      return B.CreateAdd(LHS, RHS, Name);
    case SUB:
      return B.CreateSub(LHS, RHS, Name);
    case MUL:
      return B.CreateMul(LHS, RHS, Name);
    default:;
    }
  }

  static Value *mkNum(number_t n, IntegerType *ty, LLVMContext &ctx) {
    return ConstantInt::get(ty, n.get_str(), 10);
  }

  static Value *mkVar(varname_t v) {
    return (v.get() ? const_cast<Value *>(*(v.get())) : nullptr);
  }

  static Value *mkBool(LLVMContext &ctx, bool val) {
    return ConstantInt::get(Type::getInt1Ty(ctx), (val) ? 1U : 0U);
  }

  static IntegerType *getIntType(varname_t var) {
    if (!var.get()) {
      return nullptr;
    } else {
      Type *Ty = (const_cast<Value *>(*(var.get())))->getType();
      if (IntegerType *ITy = dyn_cast<IntegerType>(Ty)) {
        return ITy;
      }
    }
    return nullptr;
  }
  
  //###############################################################################
  // function by sofia-------------------------------
  static Value *cstToValue(lin_cst_t cst, IRBuilder<> B, LLVMContext &ctx, const Twine &Name, 
                      IntegerType *ty, bool equality, bool inequality, bool strict_inequality, bool change_constant = false) {
    
    auto e = cst.expression() - cst.expression().constant();

    Value *ee = mkNum(number_t("0"), ty, ctx);
    for (auto t : e) {
      //for each term in the expression
      number_t n = t.first;
      if (n == 0){ 
        continue;
      }
      if (mayOverflow(n, ty->getBitWidth())) {
        out << "Dismissed as overflow case\n";
	      return nullptr;
      }
      varname_t v = t.second.name();
      Value *vv = mkVar(v);
      assert(vv);
      assert(vv->getType()->isIntegerTy());
      if (n == 1) {
        ee = mkBinOp(ADD, B, ee, vv, Name);
      } else if (n == -1) {
        ee = mkBinOp(SUB, B, ee, vv, Name);
      } else {
        ee = mkBinOp(ADD, B, ee,
            mkBinOp(MUL, B, mkNum(n, ty, ctx), vv, Name), Name);
      }
      assert(ee);
    }

    number_t c = -cst.expression().constant();
    // assuming linear constraints are in the form a1x1 + a2x2 +...+ constant <=/=/!=  0

    // HACK: skip constraints whose rhs that do not fit into ty
    if (mayOverflow(c, ty->getBitWidth())) {
      out << "Dismissed as overflow case\n";
      return nullptr;
    }

    Value *cc = mkNum(c, ty, ctx);

 
    if (change_constant){
      auto max = int64_t(crab::wrapint::get_signed_max(ty->getBitWidth()).get_signed_bignum());
      auto min = int64_t(crab::wrapint::get_signed_min(ty->getBitWidth()).get_signed_bignum());
      int64_t new_constant = int64_t(c);
      //change (a1x1 +...+ anxn <= c)  to (a1x1 +...+ anxn <= c + a  (or -a)) 
      
      if (Mode == "stronger"){
        // (c - a >= min)  -> (a >= -min - c) -> a >= max + c
        //max_a = max + c, overflow check:
        int64_t max_a;
        if (c>=0){
          max_a = max ;  //if we add (max + c) it will overflow
        }
        else{
          max_a = max + int64_t(c); //overflow not possible
        }

        //if max_a is a 32 bit integer
        if (max_a < 2147483647){
          new_constant -= random_normal(0, int(max_a));
        }
        else{
          new_constant -= random_normal(0, 2147483647);
        }
      }
      else{ //weaker
        // looking for a random constant a>0 :
        // (c + a <= max)  -> (a <= max - c)
        int64_t max_a = max - int64_t(c);
        //if max_a is 32bit integer (or less)
        if (max_a < 2147483647){
          new_constant += random_normal(0, int(max_a));
        }
        else{
          new_constant += random_normal(0, 2147483647);
        }
      }
      
      cc = mkNum(number_t(new_constant), ty, ctx);
      out<< "making constraint "<< Mode <<": from <="<< int64_t(c)<<" to <="<<new_constant<<"\n";
    }



    //sofia: print assumption with transformation--------------
    if(equality){
      out << "Constraint turned to equality (=) \n"; 
      return B.CreateICmpEQ(ee, cc, Name);
    }

    else if (inequality){
      bool coin = flip_a_coin(Percentage);
      if (coin){
        out << "Constraint turned to inequality (<=) \n"; 
        return B.CreateICmpSLE(ee, cc, Name);
      }
      else{
        out << "Constraint turned to inequality (>=) \n"; 
        return B.CreateICmpSGE(ee, cc, Name);
      }
    }

    else if (strict_inequality){
      out << "Constraint turned to strict inequality (<) \n"; 
      return B.CreateICmpSLT(ee, cc, Name);
    }

    //----------------------------------------------------------
    out << "Constraint as it was \n";
    if (cst.is_inequality()) {        // inequality
      return B.CreateICmpSLE(ee, cc, Name);
    } 
    else if (cst.is_equality()) {        // equality
      return B.CreateICmpEQ(ee, cc, Name);
    } 
    else {        // not equal
      return B.CreateICmpNE(ee, cc, Name);
    }
  }
  //###############################################################################

  static bool mayOverflow (const number_t &n, crab::wrapint::bitwidth_t b) {
    auto max = crab::wrapint::get_signed_max(b).get_signed_bignum();
    auto min = crab::wrapint::get_signed_min(b).get_signed_bignum();
    //out<<"max = "<<int64_t(max)<<", min = "<<int64_t(min)<<"\n";
    return (n < min || n > max);
  };



  // post: return a value of bool type(Int1Ty) that contains the
  // computation of cst
  static Value *genCode(lin_cst_t cst, IRBuilder<> B, LLVMContext &ctx,
		 DominatorTree *DT, const Twine &Name) {
    if (cst.is_tautology()) {
      return nullptr; // mkBool(ctx, true);
    }
    if (cst.is_contradiction()) {
      return mkBool(ctx, false);
    }
    // translate only expressions of LLVM integer type
    IntegerType *ty = nullptr;
    for (auto v : cst.variables()) {
      if (!ty) {
        ty = getIntType(v.name());
      } else {
        if (ty != getIntType(v.name())) {
          ty = nullptr;
          break;
        }
      }
    }
    if (!ty) {
      return nullptr;
    }
    // more sanity checks before we insert the invariants
    for (auto t : cst.expression()) {
      varname_t v = t.second.name();
      if (Value *vv = mkVar(v)) {
        // cst can contain pointer variables representing their offsets.
        // We ignore them for now.
        if (!vv->getType()->isIntegerTy()) {
          return nullptr;
        }
        if (Instruction *Def = dyn_cast<Instruction>(vv)) {
          // check definition of invariant variable dominates the
          // block where the invariant will be inserted.
          BasicBlock *User = B.GetInsertBlock();
          if (Def->getParent() == User && isa<PHINode>(Def)) {
            // definition is a PHI node and its user is in the same
            // basic block.  Since we only insert invariants after the
            // last PHI node, we are ok.
            continue;
          } else if (DT && !(DT->dominates(Def, User))) {
            // llvm::errs() << *Def << " does not dominate its use at block "
            //             << User->getName() << "\n";
            return nullptr;
          }
        }
      } else {
        return nullptr;
      }
    }
      
// CODE BY SOFIA ###################################################################
    log_info(B.GetInsertBlock(), cst);  //always log info
    
    bool transform = flip_a_coin(Percentage);
    int dice = throw_dice(2);
    if (Seed == -1) {
      transform = false;
    } 
    //========================================================================
    // STRONGER TRANSFORMATION
    //========================================================================
    if (transform && Mode == "stronger"){
      
      if (cst.is_inequality()){
        dice = throw_dice(4);
        switch (dice){
          case 0: 
          { 
            //assume False
            out << "Assume FALSE \n"; 
            Value *false_v = mkNum(number_t("0"), Type::getInt1Ty(ctx), ctx);
            return false_v; //do not print the assumption
          }
          case 1:
          { 
            // inequality (<=) to equality (=)
            return cstToValue(cst, B, ctx, Name, ty, true, false, false);
          }
          case 2:
          { 
            // inequality (<=) to strict inequality (<)
            return cstToValue(cst, B, ctx, Name, ty, false, false, true);
          }
          case 3:
          { 
            // change constant (x <= a) -> (x <= a - c)
            //currently as it is
            return cstToValue(cst, B, ctx, Name, ty, false, false, false, true);
          }
          default:
          { 
            // leave it as it is
            return cstToValue(cst, B, ctx, Name, ty, false, false, false);
          }
        }
      }
      else if (cst.is_equality()){
        //assume False
        out << "Assume FALSE \n"; 
        Value *false_v = mkNum(number_t("0"), Type::getInt1Ty(ctx), ctx);
        return false_v; //do not print the assumption
      }

      else {  //non equality
        //assume False
        out << "Assume FALSE \n"; 
        Value *false_v = mkNum(number_t("0"), Type::getInt1Ty(ctx), ctx);
        return false_v; //do not print the assumption
      }

    }   
    //========================================================================
    // WEAKER TRANSFORMATION
    //========================================================================
    else if (transform && Mode == "weaker"){
      if (cst.is_inequality()){
        dice = throw_dice(2);
        switch (dice){
          case 0:
          {
            //drop constraint // assume(true)
            out << "Assume TRUE \n"; 
            Value *true_v = mkNum(number_t("1"), Type::getInt1Ty(ctx), ctx);
            return true_v; //do not print the assumption
            //out << "Constraint Dropped\n";
            //return nullptr; //do not print the assumption
          }
          case 1:
          {
            // change constant (x <= a) -> (x <= a + c)
            //currently as it is
            return cstToValue(cst, B, ctx, Name, ty, false, false, false, true);
          }
          default:
            // leave it as it is
            return cstToValue(cst, B, ctx, Name, ty, false, false, false);
        }
      }
      else if (cst.is_equality()){
        dice = throw_dice(2);
        switch (dice){
          case 0:
          {
            //drop constraint / assume true
            out << "Assume TRUE \n"; 
            Value *true_v = mkNum(number_t("1"), Type::getInt1Ty(ctx), ctx);
            return true_v; //do not print the assumption
            //out << "Constraint Dropped\n";
            //return nullptr; //do not print the assumption
          }
          case 1:
          { 
            // turn quality (x = a) to inequality (x <= a + c)
            return cstToValue(cst, B, ctx, Name, ty, false, true, false);
          }
          default:
            // leave it as it is
            return cstToValue(cst, B, ctx, Name, ty, false, false, false);
        }
      }
      else {  //non equality
        //drop constraint / assume true
        out << "Assume TRUE \n"; 
        Value *true_v = mkNum(number_t("1"), Type::getInt1Ty(ctx), ctx);
        return true_v; //do not print the assumption
        //out << "Constraint Dropped\n";
        //return nullptr; //do not print the assumption
      }

    }
    //========================================================================
    // NO TRANSFORMATION
    //========================================================================
    else if (! transform) { 
        return cstToValue(cst, B, ctx, Name, ty, false, false, false);
    }

    else return nullptr; //in case something went wrong
//-------------------------------------------------------------------------------  
  }


  // Sofia :
  static Value *genRandomAssert(lin_cst_t cst, IRBuilder<> B, LLVMContext &ctx,
		     DominatorTree *DT, const Twine &Name = "assert_") {

    //get integer type (ty)
    IntegerType *ty = nullptr;
    for (auto v : cst.variables()) {
      if (!ty) {
        ty = getIntType(v.name());
      } else {
        if (ty != getIntType(v.name())) {
          ty = nullptr;
          break;
        }
      }
    }
    auto e = cst.expression() - cst.expression().constant();
    for (auto t : e) {
      //for each term in the expression
      number_t n = t.first;
      if (n == 0){ 
        continue;
      }
      if (mayOverflow(n, ty->getBitWidth())) {
        return nullptr;
      }

      varname_t v = t.second.name();
      Value *vv = mkVar(v);
      assert(vv);
      assert(vv->getType()->isIntegerTy());

      Value *cc = mkNum(throw_dice(100), ty, ctx);
      return B.CreateICmpEQ(vv, cc, Name);
    }
    return nullptr;
  }


  static Value *genOracleAssert(lin_cst_t cst, IRBuilder<> B, LLVMContext &ctx,
			DominatorTree *DT, const Twine &Name = "oracle_") {
    
    if (cst.is_tautology()) {
      return nullptr; // mkBool(ctx, true);
    }
    if (cst.is_contradiction()) {
      return mkBool(ctx, false);
    }
    // translate only expressions of LLVM integer type
    IntegerType *ty = nullptr;
    for (auto v : cst.variables()) {
      if (!ty) {
        ty = getIntType(v.name());
      } else {
        if (ty != getIntType(v.name())) {
          ty = nullptr;
          break;
        }
      }
    }
    if (!ty) {
      return nullptr;
    }
    // more sanity checks before we insert the invariants
    for (auto t : cst.expression()) {
      varname_t v = t.second.name();
      if (Value *vv = mkVar(v)) {
        if (!vv->getType()->isIntegerTy()) {
          return nullptr;
        }
        if (Instruction *Def = dyn_cast<Instruction>(vv)) {
          BasicBlock *User = B.GetInsertBlock();
          if (Def->getParent() == User && isa<PHINode>(Def)) {
            continue;
          } else if (DT && !(DT->dominates(Def, User))) {
            return nullptr;
          }
        }
      } else {
        return nullptr;
      }
    }
    auto e = cst.expression() - cst.expression().constant();
    Value *ee = mkNum(number_t("0"), ty, ctx);
    for (auto t : e) {
      number_t n = t.first;
      if (n == 0) {
        continue;
      }

      // HACK: skip constraints with coefficients that do not fit into ty
      if (mayOverflow(n, ty->getBitWidth())) {
	      return nullptr;
      }
      varname_t v = t.second.name();
      Value *vv = mkVar(v);
      assert(vv);
      assert(vv->getType()->isIntegerTy());
      if (n == 1) {
        ee = mkBinOp(ADD, B, ee, vv, Name);
      } else if (n == -1) {
        ee = mkBinOp(SUB, B, ee, vv, Name);
      } else {
        ee = mkBinOp(ADD, B, ee,
		     mkBinOp(MUL, B, mkNum(n, ty, ctx), vv, Name), Name);
      }
      assert(ee);
    }

    number_t c = -cst.expression().constant();
    // HACK: skip constraints whose rhs that do not fit into ty
    if (mayOverflow(c, ty->getBitWidth())) {
      return nullptr;
    }
    
    Value *cc = mkNum(c, ty, ctx);
    if (cst.is_inequality()) {
      return B.CreateICmpSLE(ee, cc, Name);
    } else if (cst.is_equality()) {
      return B.CreateICmpEQ(ee, cc, Name);
    } else {
      return B.CreateICmpNE(ee, cc, Name);
    }
  }


  //###############################################################################
  //###############################################################################

public:
  /** Generate llvm bitcode from a set of linear constraints.
   *
   * Given {x >= y, z <= 5, ...} it produces the LLVM bitcode:
   *   b1 := ICmp x geq y
   *   llvm.assume(b1)
   *   b2 := ICmp z leq 5
   *   llvm.assume(b2)
   *   ...
   **/
  bool genCode(lin_cst_sys_t csts, IRBuilder<> B, LLVMContext &ctx,
	       Function *assumeFn, CallGraph *cg, DominatorTree *DT,
	       const Function *insertFun, const Twine &Name = "") const{
    bool change = false;

    // SOFIA: 
    // do not add anything on verifier assert
    llvm::BasicBlock * BB = B.GetInsertBlock();
    llvm::Function * F = BB->getParent();
    if (F->getName() == "__VERIFIER_assert"){
      return false;
    } 

    //for each constraint
    for (auto cst : csts) {
      if (Value *cst_code = genCode(cst, B, ctx, DT, Name)) {
        CallInst *ci = B.CreateCall(assumeFn,  B.CreateZExtOrTrunc(cst_code, Type::getInt1Ty(ctx)));
        change = true;
        if (cg) {
          (*cg)[insertFun]->
	    addCalledFunction(ci, (*cg)[ci->getCalledFunction()]);
        }

        //Sofia :
        //Add random assertion with a probability
        if (flip_a_coin(AssertionPercentage) && AddAssertions == 1){
          Value *assert_val = genRandomAssert(cst, B, ctx, DT, "assert_");
          out<<"adding new assertion\n";

          if (assert_val){ 
            CallInst *ci_assert = B.CreateCall(m_assertFn, B.CreateZExtOrTrunc(assert_val, Type::getInt32Ty(ctx))); 
        
            if (cg) {
              (*cg)[insertFun]->
                addCalledFunction(ci_assert, (*cg)[ci_assert->getCalledFunction()]);
            }
          }
        }
        //Add invariant as assertion if asked so 
        if (OracleAssertions && AddAssertions == 1){
          Value *assert_val = genOracleAssert(cst, B, ctx, DT, "oracle_");
          out<<"adding oracle assertion\n";

          if (assert_val){ 
            CallInst *ci_assert = B.CreateCall(m_assertFn, B.CreateZExtOrTrunc(assert_val, Type::getInt32Ty(ctx))); 
        
            if (cg) {
              (*cg)[insertFun]->
                addCalledFunction(ci_assert, (*cg)[ci_assert->getCalledFunction()]);
            }
          }
        }


      }
    }
    return change;
  }
 
};

// Generate bitcode for the value of v if it is a constant
Constant *getConstantInt(CfgBuilder *clamCfgBuilder, clam_abstract_domain inv, const Value &v) {
  if (v.getType()->isIntegerTy()) {
    llvm::Optional<var_t> lhs = clamCfgBuilder->getCrabVariable(v);
    if (lhs.hasValue()) {
      auto interval = inv[lhs.getValue()];
      if (boost::optional<number_t> constant_opt = interval.singleton()) {
	if ((*constant_opt).fits_int64()) {
	  return ConstantInt::get(v.getType(), (int64_t)(*constant_opt), 10);	      
	}
      } else {
      CRAB_LOG("clam-opt",
	       crab::outs() << "Found crab variable " << lhs.getValue();
	       llvm::errs() << " for " << v << " ";
	       crab::outs() << "but " << interval << " is not a constant\n";);
      }
    } else {
      CRAB_LOG("clam-opt",
	       llvm::errs() << "Not found crab variable for " << v << "\n";);
    }
  }
  return nullptr;
}
  
/*
 * Reconstruct the invariants that hold after each statement. 
 *
 * For memory reasons, Crab only gives us invariants that hold either
 * at the entry or at the exit of a basic block but not at each
 * program point. Thus, we need to take the invariants that hold at
 * the entry and propagate (rebuild) them locally across the
 * statements of the basic block. This will redo some work but it's
 * more efficient than storing all invariants at each program point.
 * 
 * Op must implement two operations:
 * 
 *  bool Skip(const statement_t &)
 *  void Process(const statement_t &s, const clam_abstract_domain &inv_after_s)
 */ 
template<class Op>  
bool GenericInstrumentStatement(clam_abstract_domain inv, basic_block_t &bb, Op op) {
  // Forward propagation through the basic block but ignoring
  // callsites. This might be imprecise if the analysis was
  // inter-procedural because we cannot reconstruct all the context
  // that the inter-procedural analysis had during the analysis.
  using abs_tr_t = crab::analyzer::intra_abs_transformer<basic_block_t,
                                                         clam_abstract_domain>;
  abs_tr_t vis(inv);
  bool change = false;
  for (auto &s : bb) {
    s.accept(&vis); // propagate the invariant one statement forward
    const clam_abstract_domain &next_inv = vis.get_abs_value();
    if (next_inv.is_top())
      continue;
    if (op.Skip(s))
      continue;
    op.Process(s, next_inv);
    change = true;
  }
  return change;
}



// Convert a Crab statement to a LoadInst.
Instruction* getLoadInst(const statement_t &s) {
  using array_load_t =
    array_load_stmt<basic_block_label_t, number_t, varname_t>;
  using load_from_ref_t =
    load_from_ref_stmt<basic_block_label_t, number_t, varname_t>;

  if (s.is_arr_read()) {
    const array_load_t *load_stmt = static_cast<const array_load_t *>(&s);
    if (auto v = load_stmt->lhs().name().get()) {
      if (auto LI = dyn_cast<const LoadInst>(*v)) {
	return const_cast<LoadInst*>(LI);
      }
    }
  } else if (s.is_ref_load()) {
    const load_from_ref_t *load_stmt =
      static_cast<const load_from_ref_t *>(&s);
    if (auto v = load_stmt->lhs().name().get()) {
      if (auto LI = dyn_cast<const LoadInst>(*v)) {
	return const_cast<LoadInst*>(LI);
      } 
    }
  } 
  return nullptr;
}
  
class InstrumentLoadStmt {
  IRBuilder<> &m_IB;
  Instruction  *m_LI;
  Function  *m_assumeFn;
  CallGraph *m_cg;
public:
  InstrumentLoadStmt(IRBuilder<> &IB, Function *assumeFn, CallGraph *cg)
    : m_IB(IB), m_LI(nullptr), m_assumeFn(assumeFn), m_cg(cg) {}
  
  bool Skip(const statement_t &s) {
    m_LI = getLoadInst(s);
    return !m_LI;
  }
  
  void Process(const statement_t &s, const clam_abstract_domain &inv) {
    if (!m_LI) return; 
    
    // Filter out all irrelevant constraints
    lin_cst_sys_t rel_csts;
    std::set<var_t> rel_vars;
    rel_vars.insert(s.get_live().defs_begin(), s.get_live().defs_end());
    for (auto cst : inv.to_linear_constraint_system()) {
      std::vector<var_t> v_intersect;
      std::set_intersection(cst.variables().begin(), cst.variables().end(),
                            rel_vars.begin(), rel_vars.end(),
                            std::back_inserter(v_intersect));
      if (!v_intersect.empty()) {
        rel_csts += cst;
      }
    }

    // Insert an assume instruction after the load instruction (m_LI)
    m_IB.SetInsertPoint(m_LI);
    llvm::BasicBlock *InsertBlk = m_IB.GetInsertBlock();
    llvm::BasicBlock::iterator InsertPt = m_IB.GetInsertPoint();
    InsertPt++; // this is ok because LoadInstr cannot be terminators.
    m_IB.SetInsertPoint(InsertBlk, InsertPt);
    NumInstrLoads++;
    CodeExpander g;    
    g.genCode(rel_csts, m_IB, m_IB.getContext(), m_assumeFn, m_cg, nullptr,
	      m_LI->getParent()->getParent(), "crab_");

    // reset internal state
    m_LI = nullptr;
  }
};
  
class ConstantReplaceStmt {
  CfgBuilder *m_clamCfgBuilder;
public:
  ConstantReplaceStmt(CfgBuilder *clamCfgBuilder)
    : m_clamCfgBuilder(clamCfgBuilder) {}
  
  bool Skip(const statement_t &s) {
    if (s.is_callsite() || s.is_intrinsic() ||
	s.is_assume() || s.is_assert() ||
	s.is_havoc() ||
	s.is_int_cast()) {
      return true;
    }
    if (s.get_live().num_defs() != 1) {
      return true;
    }
    return false;
  }
  
  void Process(const statement_t &s, const clam_abstract_domain &inv) {
    assert(s.get_live().num_defs() == 1);
    if (auto v = (*(s.get_live().defs_begin())).name().get()) {
      if (const Instruction *I = dyn_cast<const Instruction>(*v)) {
	if (Constant *C = getConstantInt(m_clamCfgBuilder, inv, *I)) {
	  const_cast<Instruction*>(I)->replaceAllUsesWith(C);
	}
      }
    }
  }
};
  
} // end namespace 

namespace clam {
  
// Instrument basic block entries with a sequence of assume instructions
static bool instrumentBlock(lin_cst_sys_t csts, llvm::BasicBlock *bb,
			    LLVMContext &ctx, CallGraph *cg, DominatorTree *DT,
			    Function *assumeFn) {

  // If the block is an exit we do not instrument it.
  const ReturnInst *ret = dyn_cast<const ReturnInst>(bb->getTerminator());
  if (ret)
    return false;

  IRBuilder<> Builder(ctx);
  Builder.SetInsertPoint(bb->getFirstNonPHI());
  CodeExpander g;
  NumInstrBlocks++;
  bool res = g.genCode(csts, Builder, ctx, assumeFn, cg, DT, bb->getParent(),
                        "crab_");

  return res;
}

// Instrument LoadInst with a sequence of assume instructions  
static bool instrumentLoadInst(clam_abstract_domain inv, basic_block_t &bb,
			       LLVMContext &ctx, CallGraph *cg,
			       Function *assumeFn) {
  IRBuilder<> Builder(ctx);
  InstrumentLoadStmt ILS(Builder, assumeFn, cg);
  return GenericInstrumentStatement(inv, bb, ILS);
}

// Do constant replacement  
static bool constantReplacement(CfgBuilder *clamCfgBuilder,
				clam_abstract_domain inv,
				basic_block_t &bb) {
  ConstantReplaceStmt CRS(clamCfgBuilder);
  return GenericInstrumentStatement(inv, bb, CRS);
}

// Identify whether B is dead and which B's successor edges are dead.
static bool markDeadBlocksAndEdges(ClamGlobalAnalysis  &clam,
				   BasicBlock &B,
				   std::vector<BasicBlock*> &deadBlocks,
				   std::vector<std::pair<BasicBlock*,BasicBlock*>> &deadEdges) {
  Function &F = *(B.getParent());
  // Mark dead successor edges
  for (BasicBlock *Succ : successors(&B)) {
    if (!clam.hasFeasibleEdge(&B, Succ)) {
      CRAB_LOG("clam-opt",
	       llvm::errs() << "clam-opt detected dead"
	       << " edge between block "
	       << B.getName () << " and " << Succ->getName()
	       << " in function " << F.getName() << "\n";);
      deadEdges.push_back({&B, Succ});
    }
  }

  // Mark whether the block is dead
  llvm::Optional<clam_abstract_domain> pre = 
    clam.getPre(&B, false /*do not keep ghost variables*/);    
  if (pre.hasValue()) {
    if (pre.getValue().is_bottom()) {
      CRAB_LOG("clam-opt",
	       llvm::errs() << "clam-opt detected dead block "
	       << B.getName () << " in function "
	       << F.getName() << "\n";);
      deadBlocks.push_back(&B);
      return true;
    }
  }
  return false;
}

// Remove a block  
static void removeDeadBlock(BasicBlock *BB, LLVMContext &ctx) {
  ++NumDeadBlocks;
  // Loop through all of our successors and make sure they know that one
  // of their predecessors is going away.
  for (BasicBlock *Succ : successors(BB)) {
    Succ->removePredecessor(BB);
  }
  // Zap all the instructions in the block.
  while (!BB->empty()) {
    Instruction &I = BB->back();
    // If this instruction is used, replace uses with an arbitrary value.
    // Because control flow can't get here, we don't care what we replace the
    // value with.  Note that since this block is unreachable, and all values
    // contained within it must dominate their uses, that all uses will
    // eventually be removed (they are themselves dead).
    if (!I.use_empty()) {
      I.replaceAllUsesWith(UndefValue::get(I.getType()));
    }
    BB->getInstList().pop_back();
  }
  // Add unreachable terminator
  BB->getInstList().push_back(new UnreachableInst(ctx));
}

// Remove an edge  
static void removeDeadEdge(BasicBlock *BB, BasicBlock *Succ) {
  ++NumDeadEdges;

  Instruction *TI = BB->getTerminator();
  if (BranchInst *BI = dyn_cast<BranchInst>(TI)) {
    if (BI->isConditional()) {
      // Replace conditional branch with an unconditional one.
      BasicBlock *OnlySucc = nullptr;
      for (unsigned i = 0, e = BI->getNumSuccessors(); i < e; ++i) {
        if (BI->getSuccessor(i) == Succ) {
          continue;
        } else if (!OnlySucc) {
          OnlySucc = BI->getSuccessor(i);
        } else {
          OnlySucc = nullptr;
        }
      }
      if (!OnlySucc)
        return;

      BranchInst *NewTI = BranchInst::Create(OnlySucc, TI);
      TI->replaceAllUsesWith(NewTI);
      TI->eraseFromParent();

      // Fix phi nodes of the successor.
      std::vector<std::pair<PHINode *, unsigned>> PHIValues;
      for (PHINode &PHI : Succ->phis()) {
        for (unsigned i = 0, e = PHI.getNumIncomingValues(); i < e; ++i) {
          if (PHI.getIncomingBlock(i) == BB) {
            PHIValues.push_back({&PHI, i});
          }
        }
      }

      for (auto &p : PHIValues) {
        PHINode *PHI = p.first;
        unsigned i = p.second;
        PHI->removeIncomingValue(i);
      }
    }
  }
}
   
Optimizer::Optimizer(ClamGlobalAnalysis  &clam,
		     llvm::CallGraph *callgraph,
		     std::function<llvm::DominatorTree*(llvm::Function*)> DT,
		     std::function<llvm::LoopInfo*(llvm::Function*)> LI,
		     InvariantsLocation addInvariants,
		     bool removeDeadCode,
		     bool replaceWithConstants)
  : m_clam(clam)
  , m_cg(callgraph)
  , m_dt(DT)
  , m_li(LI)
  , m_invLoc(addInvariants)
  , m_removeDeadCode(removeDeadCode)
  , m_replaceWithConstants(replaceWithConstants)
  , m_assumeFn(nullptr) {}

  
bool Optimizer::runOnModule(Module &M) {
  if (m_invLoc == InvariantsLocation::NONE &&
      !m_removeDeadCode &&
      !m_replaceWithConstants) {
    return false;
  }

  CRAB_VERBOSE_IF(1, crab::get_msg_stream()
		  << "Starting clam optimizer based on invariants.\n";);


  //CODE BY SOFIA-------------------------------------------------------
  std::string log_file = Subfolder + "/log.txt";
  out.open(log_file);
  out<<"-------------------------------------------------\n";
  out<<"SEED : "<< Seed <<"\n";
  out<<"ASSERT PERCENTAGE : "<<AssertionPercentage<<"%\n";
  out<<"PERCENTAGE : "<< Percentage <<"%\n";
  out<<"TRANSFORMATION MODE : "<< Mode <<"\n";
   out<<"-------------------------------------------------\n\n";
  //--------------------------------------------------------------------

  LLVMContext &ctx = M.getContext();
  AttrBuilder B;
  B.addAttribute(Attribute::NoUnwind);
  B.addAttribute(Attribute::NoRecurse);
  B.addAttribute(Attribute::OptimizeNone);  
  // LLVM removed all calls to verifier.assume if marked as ReadNone
  // or ReadOnly even if we mark it as OptimizeNone.
  B.addAttribute(Attribute::InaccessibleMemOnly);  

  if (Smack){
    B.addAttribute(Attribute::NoInline);
    //nessesary for llvm 12, when OptimizeNone is used
  }

  AttributeList as = AttributeList::get(ctx, AttributeList::FunctionIndex, B);
  m_assumeFn = dyn_cast<Function>(M.getOrInsertFunction("verifier.assume", as,
                                                        Type::getVoidTy(ctx),
                                                        Type::getInt1Ty(ctx))
                                      .getCallee());
  if (m_cg) {
    m_cg->getOrInsertFunction(m_assumeFn);
  }
  
  // Sofia : get or Insert Assert function:  
 /* 
  AttrBuilder B2; 
  AttributeList as2 = AttributeList::get(ctx, AttributeList::FunctionIndex, B2);
  m_assertFn = dyn_cast<Function>(M.getOrInsertFunction("__CRAB_assert", as2,
                                                        Type::getVoidTy(ctx),
                                                        Type::getInt32Ty(ctx))
                                      .getCallee());
*/
  // assertions crab understands: verifier.assert, crab.assert, __VERIFIER_assert, __CRAB_assert
  //sos assertions NOT woking  for __VERIFIER_assert & Int1
  //more info on lib/Clam/CfgBuilderUtils.cc
  
  AttrBuilder B2; 
  //B2.addAttribute(Attribute::ReadNone);
  AttributeList as2 = AttributeList::get(ctx, AttributeList::FunctionIndex, B2);
  m_assertFn = dyn_cast<Function>(M.getOrInsertFunction("__VERIFIER_assert", as2,
                                                        Type::getVoidTy(ctx),
                                                        Type::getInt32Ty(ctx))
                                      .getCallee());
  //mdkw

  if (m_cg) {
    m_cg->getOrInsertFunction(m_assertFn);
  }


  bool change = false;
  
  for (auto &f : M) {
    change |= runOnFunction(f);
  }

  CRAB_VERBOSE_IF(1, crab::get_msg_stream()
		  << "Finished clam optimizer based on invariants.\n";); 

 
  //CODE BY SOFIA------------------------------------------------------
  out.close();
  //--------------------------------------------------------------------
     
  return change;
}

bool Optimizer::runOnFunction(Function &F) {
  if (F.empty() || F.isVarArg()) {
    return false;
  }

  CRAB_VERBOSE_IF(1, crab::get_msg_stream()
		  << "Started clam optimizer for " << F.getName().str() << ".\n";);
  
  if (!m_clam.getCfgBuilderMan().hasCfg(F)) {
    // This shouldn't happen
    llvm::errs() << "Crab CFG not found for " << F.getName() << "\n";
    return false;
  }

  cfg_ref_t cfg = m_clam.getCfgBuilderMan().getCfg(F);
  DominatorTree *dt = m_dt(&F); // it can be nullptr
  LLVMContext &ctx = F.getContext();
  std::vector<BasicBlock *> DeadBlocks;
  std::vector<std::pair<BasicBlock *, BasicBlock *>> DeadEdges;
  bool change = false;  
  for (auto &B : F) {
    if (hasUnreachable(B)) {
      continue;
    }

    CRAB_LOG("clam-opt2",
	     llvm::errs() << "clam-opt processing " << B.getName() << "\n";);
    
    if (m_removeDeadCode &&
	markDeadBlocksAndEdges(m_clam, B, DeadBlocks, DeadEdges)) {
      // do not keep processing the block because it is dead
      continue;
    }
    
    if (m_invLoc == InvariantsLocation::BLOCK ||
	m_invLoc == InvariantsLocation::LOOP_HEADER || 
	m_invLoc == InvariantsLocation::ALL) {
      const bool keep_ghost = false;
      llvm::Optional<clam_abstract_domain> pre = m_clam.getPre(&B, keep_ghost);
      if (pre.hasValue()) {
	if (m_invLoc == InvariantsLocation::BLOCK ||
	    m_invLoc == InvariantsLocation::ALL) {
	  auto csts = pre.getValue().to_linear_constraint_system();
	  change |= instrumentBlock(csts, &B, F.getContext(), m_cg, dt, m_assumeFn);
	} else {
	  assert(m_invLoc == InvariantsLocation::LOOP_HEADER);
	  LoopInfo *LI = m_li(&F); // it can be nullptr
	  if (LI && LI->isLoopHeader(&B)) {
	    auto csts = pre.getValue().to_linear_constraint_system();
	    change |= instrumentBlock(csts, &B, F.getContext(), m_cg, dt, m_assumeFn);
	  }
	}
      }
    }
    
    if ((m_invLoc == InvariantsLocation::LOAD_INST ||
	 m_invLoc == InvariantsLocation::ALL) && readMemory(B)) {
      const bool keep_ghost = true;
      llvm::Optional<clam_abstract_domain> pre = m_clam.getPre(&B, keep_ghost);
      if (pre.hasValue()) {
	auto cfg_builder_ptr = m_clam.getCfgBuilderMan().getCfgBuilder(F);
	assert(cfg_builder_ptr);
	basic_block_label_t bb_label = cfg_builder_ptr->getCrabBasicBlock(&B);
	change |= instrumentLoadInst(pre.getValue(), cfg.get_node(bb_label),
				     F.getContext(), m_cg, m_assumeFn);
      }
    }

    if (m_replaceWithConstants) {
      const bool keep_ghost = false;
      llvm::Optional<clam_abstract_domain> pre = m_clam.getPre(&B, keep_ghost);
      if (pre.hasValue()) {
	auto cfg_builder_ptr = m_clam.getCfgBuilderMan().getCfgBuilder(F);
	assert(cfg_builder_ptr);	
	basic_block_label_t bb_label = cfg_builder_ptr->getCrabBasicBlock(&B);
	change |= constantReplacement(cfg_builder_ptr, pre.getValue(), cfg.get_node(bb_label));
      }
    }
  }

  change = (!DeadEdges.empty() || !DeadBlocks.empty());
  
  // The actual removal of edges and blocks
  while (!DeadEdges.empty()) {
    std::pair<BasicBlock *, BasicBlock *> E = DeadEdges.back();
    DeadEdges.pop_back();
    removeDeadEdge(E.first, E.second);
  }

  
  while (!DeadBlocks.empty()) {
    BasicBlock *B = DeadBlocks.back();
    DeadBlocks.pop_back();
    removeDeadBlock(B, ctx);
  }

  CRAB_VERBOSE_IF(1, crab::get_msg_stream()
		  << "Finished clam optimizer for " << F.getName().str() << ".\n";);
  return change;
}
  
/* Pass code starts here */

// changed by Jorge-------------------------------------------------------
OptimizerPass::OptimizerPass():
  ModulePass(ID), m_impl(nullptr), m_clam(nullptr) {}

OptimizerPass::OptimizerPass(ClamGlobalAnalysis *clam):
  ModulePass(ID), m_impl(nullptr), m_clam(clam) {}
//------------------------------------------------------------------------
  
bool OptimizerPass::runOnModule(Module &M) {
  if (InvLoc == InvariantsLocation::NONE &&
      !RemoveDeadCode &&
      !ReplaceWithConstants) {
    return false;
  }
  
  // Get module's callgraph
  CallGraph *cg = nullptr;
  if (CallGraphWrapperPass *cgwp = getAnalysisIfAvailable<CallGraphWrapperPass>()) {
    cg = &cgwp->getCallGraph();
  }

  //added by jorge-----------------------------------------------
  // Get clam
  if (!m_clam) {
     m_clam = &(getAnalysis<ClamPass>().getClamGlobalAnalysis());
  }
  //---------------------------------------------------------------

  // Collect all the dominator tree and loop info in maps
  DenseMap<Function*, DominatorTree*> dt_map;
  DenseMap<Function*, LoopInfo*> li_map;
  for (auto &F: M) {
    if (F.empty()) continue;
    if (requireDominatorTree(InvLoc))
      dt_map[&F] = &(getAnalysis<DominatorTreeWrapperPass>(F).getDomTree());
    if (requireLoopInfo(InvLoc))    
      li_map[&F] = &(getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo());
  }


  m_impl.reset(new Optimizer(*m_clam, cg,
			     [&dt_map](Function *F) {
			       auto it = dt_map.find(F);
			       return (it != dt_map.end() ? it->second : nullptr);
			     },
			     [&li_map](Function *F) {
			       auto it = li_map.find(F);
			       return (it != li_map.end() ? it->second : nullptr);
			     },
			     InvLoc, RemoveDeadCode, ReplaceWithConstants));
  return m_impl->runOnModule(M);
}

void OptimizerPass::getAnalysisUsage(AnalysisUsage &AU) const {
  //changed by Jorge----------------------------------------
  if (!m_clam) {
     AU.addRequired<clam::ClamPass>();
  }
  //---------------------------------------------------------
  AU.addRequired<UnifyFunctionExitNodes>();
  AU.addRequired<CallGraphWrapperPass>();
  AU.addPreserved<CallGraphWrapperPass>();
  if (requireDominatorTree(InvLoc)) {
    AU.addRequired<DominatorTreeWrapperPass>();
  }
  if (requireLoopInfo(InvLoc)) {
    AU.addRequired<LoopInfoWrapperPass>();
  }
}

char clam::OptimizerPass::ID = 0;


// modified by Jorge---------------------------------------------------
llvm::Pass *createOptimizerPass(ClamGlobalAnalysis *clam = nullptr, int seed=0, int add_assertions=0, std::string mode="stronger", std::string subfolder="") {
  std::srand(seed);   //probably working, added by sofia
  Seed = seed;                        // sofia
  Mode = mode;                        // sofia
  AddAssertions = add_assertions;     // sofia
  Subfolder = subfolder;              // sofia
  return new OptimizerPass(clam);
}
 
} // namespace clam

static RegisterPass<clam::OptimizerPass>
X("crab-opt",
  "Optimize LLVM bitcode using invariants inferred by crab",
  false, false);

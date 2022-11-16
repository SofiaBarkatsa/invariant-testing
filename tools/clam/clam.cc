///
// Clam -- Abstract Interpretation-based Analyzer for LLVM bitcode
///

#include "clam/config.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO.h"

#include "clam/Clam.hh"
#include "clam/Passes.hh"

#include "seadsa/InitializePasses.hh"
#include "seadsa/support/RemovePtrToInt.hh"

#include "seadsa/AllocWrapInfo.hh"
#include "seadsa/DsaLibFuncInfo.hh"
#include "clam/CfgBuilder.hh"
#include "clam/HeapAbstraction.hh"
#include "clam/SeaDsaHeapAbstraction.hh"
#include "clam/Support/NameValues.hh"


#include <type_traits>
#include <stdlib.h>  
#include <string> 
#include <fstream>
#include "llvm/Transforms/Utils/Cloning.h"

extern llvm::cl::OptionCategory ClamOptCat;

// REVISIT
// Add here more user options for choosing clam abstract domain or other analysis options

static llvm::cl::opt<std::string>
    InputFilename(llvm::cl::Positional,
                  llvm::cl::desc("<input LLVM bitcode file>"),
                  llvm::cl::Required, llvm::cl::value_desc("filename"),
		  llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<std::string>
    OutputFilename("o", llvm::cl::desc("Override output filename"),
                   llvm::cl::init(""), llvm::cl::value_desc("filename"),
		   llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool>
OutputAssembly("S", llvm::cl::desc("Write output as LLVM assembly"),
	       llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<std::string>
    AsmOutputFilename("oll", llvm::cl::desc("Output analyzed bitcode"),
                      llvm::cl::init(""), llvm::cl::value_desc("filename"),
		      llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<std::string> DefaultDataLayout(
    "default-data-layout",
    llvm::cl::desc("data layout string to use if not specified by module"),
    llvm::cl::init(""), llvm::cl::value_desc("layout-string"),
    llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool> DisableCrab(
    "no-crab",
    llvm::cl::desc("Output preprocessed bitcode but disabling Crab analysis"),
    llvm::cl::init(false), llvm::cl::Hidden,
    llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool> TurnUndefNondet(
    "clam-turn-undef-nondet",
    llvm::cl::desc("Turn undefined behaviour into non-determinism"),
    llvm::cl::init(false),
    llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool>
    LowerUnsignedICmp("clam-lower-unsigned-icmp",
                      llvm::cl::desc("Lower ULT and ULE instructions"),
                      llvm::cl::init(false),
		      llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool>
    LowerCstExpr("clam-lower-constant-expr",
                 llvm::cl::desc("Lower constant expressions to instructions"),
                 llvm::cl::init(true),
		 llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool>
    LowerInvoke("clam-lower-invoke",
                llvm::cl::desc("Lower invoke instructions"),
                llvm::cl::init(true),
		llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool>
    LowerSwitch("clam-lower-switch",
                llvm::cl::desc("Lower switch instructions"),
                llvm::cl::init(true),
		llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool>
    LowerSelect("clam-lower-select",
                llvm::cl::desc("Lower all select instructions"),
                llvm::cl::init(false),
		llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool>
    CrabOpt("crab-opt",
	    llvm::cl::desc("Optimize LLVM bitcode by using invariants"),
	    llvm::cl::init(false),
	    llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool> PromoteAssume(
    "crab-promote-assume",
    llvm::cl::desc("Promote verifier.assume to llvm.assume intrinsics"),
    llvm::cl::init(false),
    llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool> DotLLVMCFG(
    "clam-llvm-cfg-dot",
    llvm::cl::desc("Write a .dot file the analyzed LLVM CFG of each function"),
    llvm::cl::init(false),
    llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool>
    NullCheck("clam-null-check-legacy",
              llvm::cl::desc("Insert checks for null dereference errors in LLVM IR"),
              llvm::cl::init(false),
	      llvm::cl::cat(ClamOptCat));

static llvm::cl::opt<bool>
    UafCheck("clam-uaf-check-legacy",
             llvm::cl::desc("Insert checks for use-after-free errors in LLVM IR"),
             llvm::cl::init(false),
	     llvm::cl::cat(ClamOptCat));


// PARAMETERS BY SOFIA ----------------------------------------------------------------------------
static llvm::cl::opt<int>
    NumOfFiles("num_of_files",
             llvm::cl::desc("Percentage for Transformations"),
             llvm::cl::init(2));

static llvm::cl::opt<std::string>
    InvFolder("inv_folder",
             llvm::cl::desc("Invariant folder coresponding to the core calling clam"));

static llvm::cl::opt<std::string>
    Domain("domain",
             llvm::cl::desc("Abstruct domain to call clam analysis with"));

static llvm::cl::opt<std::string>
    CTrack("c_track",
             llvm::cl::desc("Crab-track = {num, mem, ...}"));

static llvm::cl::opt<std::string>
    CAnalysis("c_analysis",
             llvm::cl::desc("Inter, Intra or Backward"));

static llvm::cl::opt<int>
    CWideningDelay("c_widening_delay",
             llvm::cl::desc("c_widening_delay"),
             llvm::cl::init(1));

static llvm::cl::opt<int>
    CNarrowingIterations("c_narrowing_iterations",
             llvm::cl::desc("c_narrowing_iterations"),
             llvm::cl::init(10));

static llvm::cl::opt<int>
    CWideningJumpSet("c_widening_jump_set",
             llvm::cl::desc("c_widening_jump_set"),
             llvm::cl::init(0));

static llvm::cl::opt<bool>
    ProduceOneFile("produce_one_file",
             llvm::cl::desc("produce_one_file"),
             llvm::cl::init(false));

std::string InitialOutputFilename;
//-------------------------------------------------------------------------------------------------


using namespace clam;

// removes extension from filename if there is one
std::string getFileName(const std::string &str) {
  std::string filename = str;
  size_t lastdot = str.find_last_of(".");
  if (lastdot != std::string::npos)
    filename = str.substr(0, lastdot);
  return filename;
}

std::unique_ptr<CrabBuilderManager> mkCrabBuilderManager(llvm::Module &module,
							 llvm::TargetLibraryInfoWrapperPass &TLIW) {

  //////////////////////////////////////
  // Run seadsa -- pointer analysis
  //////////////////////////////////////  
  llvm::CallGraph cg(module);
  seadsa::AllocWrapInfo allocWrapInfo(&TLIW);
  allocWrapInfo.initialize(module, nullptr);
  seadsa::DsaLibFuncInfo dsaLibFuncInfo;
  dsaLibFuncInfo.initialize(module);
  std::unique_ptr<HeapAbstraction> mem(new SeaDsaHeapAbstraction(
		module, cg, TLIW, allocWrapInfo, dsaLibFuncInfo, true));

  //////////////////////////////////////  
  // Create CrabIR from LLVM 
  //////////////////////////////////////
  
  /// Translation from LLVM to CrabIR
  CrabBuilderParams cparams;
  // Translate all memory operations using seadsa
  //cparams.setPrecision(clam::CrabBuilderPrecision::MEM);
  // Deal with unsigned comparisons
  
  cparams.lowerUnsignedICmpIntoSigned();
  //==============================================================//  
  // REVISIT: we might need to set more CFG builder options here  Sofia:
  // look at include/clam/CFGBuilderParams.hh
  //==============================================================//  

  // Translate all memory operations using seadsa
  if (CTrack == "mem")
    cparams.setPrecision(clam::CrabBuilderPrecision::MEM);
  else if (CTrack == "num")
    cparams.setPrecision(clam::CrabBuilderPrecision::NUM);
  else  //sing-mem
    cparams.setPrecision(clam::CrabBuilderPrecision::SINGLETON_MEM);


  //==================================================================
  std::unique_ptr<CrabBuilderManager> man =
    std::make_unique<CrabBuilderManager>(cparams, TLIW, std::move(mem));
  return std::move(man);
}


std::unique_ptr<ClamGlobalAnalysis> mkClamGlobalAnalysis(llvm::Module &M, CrabBuilderManager &man) {
  
  /// Set Crab parameters
  AnalysisParams aparams;

  // Check for assertions
  aparams.check = clam::CheckerKind::ASSERTION;
  
  //==============================================================//
  // REVISIT: we might need to set more analysis options here  Sofia:
  //==============================================================//  
  
  if (Domain == "int")
    aparams.dom = CrabDomain::INTERVALS;
  else if (Domain == "w-int")
    aparams.dom = CrabDomain::WRAPPED_INTERVALS;
  else if (Domain == "boxes")
    aparams.dom = CrabDomain::BOXES;
  else if (Domain == "zones")
    aparams.dom = CrabDomain::ZONES_SPLIT_DBM;
  else if (Domain == "soct")
    aparams.dom = CrabDomain::OCT_SPLIT_DBM;
  else { 
    llvm::errs()<<"Sofia: Domain : "<<Domain<<" is not supported yet. Switching to zones\n";
    aparams.dom = CrabDomain::ZONES_SPLIT_DBM;
  }

  if (CAnalysis == "inter"){
  // Enable inter-procedural analysis
    aparams.run_inter = true;    
  }
  else if (CAnalysis == "backward"){
  // Enable backward-procedural analysis
    aparams.run_backward = true; 
  }

  aparams.widening_delay = CWideningDelay;
  aparams.narrowing_iters = CNarrowingIterations;
  aparams.widening_jumpset = CWideningJumpSet;

  llvm::errs()<<"CWideningDelay="<<CWideningDelay<<", CNarrowingIterations="<<CNarrowingIterations<<", CWideningJumpSet="<<CWideningJumpSet<<"\n";

  if (CAnalysis == "inter"){ 
    /// Create an inter-analysis instance 
    std::unique_ptr<ClamGlobalAnalysis> ga = std::make_unique<InterGlobalClam>(M, man);
    /// Run the Crab analysis
    ClamGlobalAnalysis::abs_dom_map_t assumptions;
    ga->analyze(aparams, assumptions);
    return std::move(ga);
  }
  else {  
    //llvm::errs()<<CAnalysis<<"\n";

    /// Create an intra-analysis instance 
    std::unique_ptr<ClamGlobalAnalysis> ga = std::make_unique<IntraGlobalClam>(M, man);
    /// Run the Crab analysis
    ClamGlobalAnalysis::abs_dom_map_t assumptions;
    ga->analyze(aparams, assumptions);
    return std::move(ga);
  }
}



int main(int argc, char **argv) {
  llvm::llvm_shutdown_obj shutdown; // calls llvm_shutdown() on exit

  //llvm::cl::HideUnrelatedOptions(ClamOptCat);  
  llvm::cl::ParseCommandLineOptions(
      argc, argv,
      "Clam -- Abstract Interpretation-based Analyzer of LLVM bitcode\n");

  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
  llvm::PrettyStackTraceProgram PSTP(argc, argv);
  llvm::EnableDebugBuffering = true;

  std::error_code error_code;
  llvm::SMDiagnostic err;
  static llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::ToolOutputFile> output;
  std::unique_ptr<llvm::ToolOutputFile> asmOutput;

  module = llvm::parseIRFile(InputFilename, err, context);
  if (!module) {
    if (llvm::errs().has_colors())
      llvm::errs().changeColor(llvm::raw_ostream::RED);
    llvm::errs() << "error: "
                 << "Bitcode was not properly read; " << err.getMessage()
                 << "\n";
    if (llvm::errs().has_colors())
      llvm::errs().resetColor();
    return 3;
  }

  if (!AsmOutputFilename.empty())
    asmOutput = std::make_unique<llvm::ToolOutputFile>(
        AsmOutputFilename.c_str(), error_code, llvm::sys::fs::F_Text);
  if (error_code) {
    if (llvm::errs().has_colors())
      llvm::errs().changeColor(llvm::raw_ostream::RED);
    llvm::errs() << "error: Could not open " << AsmOutputFilename << ": "
                 << error_code.message() << "\n";
    if (llvm::errs().has_colors())
      llvm::errs().resetColor();
    return 3;
  }

  if (!OutputFilename.empty())
    output = std::make_unique<llvm::ToolOutputFile>(
        OutputFilename.c_str(), error_code, llvm::sys::fs::F_None);

  if (error_code) {
    if (llvm::errs().has_colors())
      llvm::errs().changeColor(llvm::raw_ostream::RED);
    llvm::errs() << "error: Could not open " << OutputFilename << ": "
                 << error_code.message() << "\n";
    if (llvm::errs().has_colors())
      llvm::errs().resetColor();
    return 3;
  }

  ///////////////////////////////
  // initialise and run passes //
  ///////////////////////////////

  llvm::legacy::PassManager pass_manager;
  llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeCore(Registry);
  llvm::initializeTransformUtils(Registry);
  llvm::initializeAnalysis(Registry);

  /// call graph and other IPA passes
  // llvm::initializeIPA (Registry);
  // XXX: porting to 3.8
  llvm::initializeCallGraphWrapperPassPass(Registry);
  // XXX: commented while porting to 5.0
  // llvm::initializeCallGraphPrinterPass(Registry);
  llvm::initializeCallGraphViewerPass(Registry);
  // XXX: not sure if needed anymore
  llvm::initializeGlobalsAAWrapperPassPass(Registry);

  llvm::initializeAllocWrapInfoPass(Registry);
  llvm::initializeAllocSiteInfoPass(Registry);
  llvm::initializeRemovePtrToIntPass(Registry);
  llvm::initializeDsaAnalysisPass(Registry);
  llvm::initializeDsaInfoPassPass(Registry);
  llvm::initializeCompleteCallGraphPass(Registry);

  // add an appropriate DataLayout instance for the module
  const llvm::DataLayout *dl = &module->getDataLayout();
  if (!dl && !DefaultDataLayout.empty()) {
    module->setDataLayout(DefaultDataLayout);
    dl = &module->getDataLayout();
  }

  assert(dl && "Could not find Data Layout for the module");

  /**
   * Here only passes that are strictly necessary to avoid crashes or
   * too poor results. Passes that are only for improving precision
   * should be run in clam-pp.
   **/

  // kill unused internal global
  pass_manager.add(llvm::createGlobalDCEPass());
  pass_manager.add(clam::createRemoveUnreachableBlocksPass());

  // -- promote alloca's to registers
  pass_manager.add(llvm::createPromoteMemoryToRegisterPass());
  if (TurnUndefNondet) {
    // -- Turn undef into nondet
    pass_manager.add(clam::createNondetInitPass());
  }
  if (LowerInvoke) {
    // -- lower invoke's
    pass_manager.add(llvm::createLowerInvokePass());
    // cleanup after lowering invoke's
    pass_manager.add(llvm::createCFGSimplificationPass());
  }
  // -- ensure one single exit point per function
  pass_manager.add(llvm::createUnifyFunctionExitNodesPass());
  // -- remove unreachable blocks
  pass_manager.add(clam::createRemoveUnreachableBlocksPass());
  if (LowerSwitch) {
    // -- remove switch constructions
    pass_manager.add(llvm::createLowerSwitchPass());
    // cleanup after lowering switches
    pass_manager.add(llvm::createCFGSimplificationPass());
  }
  // -- lower constant expressions to instructions
  if (LowerCstExpr) {
    pass_manager.add(clam::createLowerCstExprPass());
    // cleanup after lowering constant expressions
    pass_manager.add(llvm::createDeadCodeEliminationPass());
  }
  if (TurnUndefNondet) {
    pass_manager.add(clam::createDeadNondetElimPass());
  }

  // -- lower ULT and ULE instructions
  if (LowerUnsignedICmp) {
    pass_manager.add(clam::createLowerUnsignedICmpPass());
    // cleanup unnecessary and unreachable blocks
    pass_manager.add(llvm::createCFGSimplificationPass());
    pass_manager.add(clam::createRemoveUnreachableBlocksPass());
  }

  // -- remove ptrtoint and inttoptr instructions
  pass_manager.add(seadsa::createRemovePtrToIntPass());

  // -- must be the last ones before running crab.
  if (LowerSelect) {
    pass_manager.add(clam::createLowerSelectPass());
  }

  // -- ensure one single exit point per function
  //    LowerUnsignedICmpPass and LowerSelect can add multiple
  //    returns.
  pass_manager.add(llvm::createUnifyFunctionExitNodesPass());

  if (!DisableCrab) {
    /// -- Add some properties to check
    if (NullCheck)
      pass_manager.add(clam::createNullCheckPass());
    if (UafCheck)
      pass_manager.add(clam::createUseAfterFreeCheckPass());
    /// -- run the crab analyzer
    //clam = new clam::ClamPass();
    
    // run Claam Analysis only if Optimizer is not calles
    if (!CrabOpt){ 
      pass_manager.add(new clam::ClamPass()); // pass_manager owns clam!  
      if (DotLLVMCFG)
        pass_manager.add(createAnnotatedCFGPrinterPass());
    }
  }

  //#####################  sofia
  if (PromoteAssume) {
    // -- promote verifier.assume to llvm.assume intrinsics
    pass_manager.add(clam::createPromoteAssumePass());
  }
  //##############################

  pass_manager.add(new clam::NameValues());
  
  if (!OutputFilename.empty()) {
    if (OutputAssembly)
      pass_manager.add(createPrintModulePass(output->os()));
    else
      pass_manager.add(createBitcodeWriterPass(output->os()));
  }

  if (!AsmOutputFilename.empty()) {
    pass_manager.add(createPrintModulePass(asmOutput->os()));
  } 
  
  pass_manager.run(*module.get());

  if (!AsmOutputFilename.empty())  
    asmOutput->keep();
  if (!OutputFilename.empty())
    output->keep();
  
  if (!CrabOpt)
    return 0;

  // if CrabOpt NOT enabled, then return here-------------------- Sofia --------

  //---------------------------------------------------------------------------

  // At this point we have run (using pass_manager) all the passes
  // that should be run before ClamPass.
  // 
  // Now instead of running Clam via a LLVM pass (i.e., ClamPass) we
  // directly create an instance of ClamGlobalAnalysis and run it on
  // the module. This is needed because we want to extend the lifetime
  // of this Clam analysis object until the end of this program so
  // that we can pass it to createOptimizerPass. The method "run" of
  // pass_manager deletes all the analysis passed via the "add" method
  // upon completion so that's why we cannot run Clam as a LLVM pass
  // if we want to use it later.
  const auto& tripleS = module->getTargetTriple();
  llvm::Twine tripleT(tripleS);
  llvm::Triple triple(tripleT);
  llvm::TargetLibraryInfoWrapperPass  TLIW(triple);  
  std::unique_ptr<CrabBuilderManager> clam_man = mkCrabBuilderManager(*module.get(), TLIW);
  std::unique_ptr<ClamGlobalAnalysis> clam_analysis = mkClamGlobalAnalysis(*module.get(), *clam_man);


  /// Jorge: this is just to show that we can get the invariants from clam_analysis
  /*crab::outs() << "===Invariants at the entry of each block===\n";
  for (auto &f: *module.get()) {
    for (auto &b: f) {
      llvm::Optional<clam_abstract_domain> dom = clam_analysis->getPre(&b, false);
      if (dom.hasValue()) {
        crab::outs() << f.getName() << "#" << b.getName() << ":\n  " << dom.getValue() << "\n";
        crab::outs() << f.getName() << "#" << b.getName() << ":\n  " << dom.getValue().to_linear_constraint_system() << "\n";
      }
    }
  }
  crab::outs() <<"\n\n";*/

// test Numair's idea##############################
  std::unique_ptr<llvm::ToolOutputFile> output3;
  std::string OutputFilename1;

    if (!OutputFilename.empty()){ 
      OutputFilename1 = InvFolder + "/initial.bc";
      llvm::errs()<<"Output in file: "<<OutputFilename1<<"\n";
    }
  
  if (!OutputFilename.empty()){ 
    output3 = std::make_unique<llvm::ToolOutputFile>(
      OutputFilename1.c_str(), error_code, llvm::sys::fs::F_None);
  }

  llvm::legacy::PassManager pass_manager2;  
  
  pass_manager2.add(clam::createDeleteAssumePass());
  pass_manager2.add(clam::createDeleteCrabCommandPass());
  
  if (!DisableCrab && CrabOpt) {
    // post-processing of the bitcode using Crab invariants
    pass_manager2.add(clam::createOptimizerPass(&(*clam_analysis), -1, "weaker", InvFolder));
  }

  if (!OutputFilename.empty()) {
      pass_manager2.add(createBitcodeWriterPass(output3->os()));
  }

  pass_manager2.run(*module.get()); 

  if (!OutputFilename.empty()){ 
    output3->keep();
  }
//end of Numair's idea ################################
  
  //in case we want to skip producing more than one files
  if (ProduceOneFile == true){
      return 0;
  }

  //modifications by sofia if CrabOpt enabled
  for (int i=0; i<NumOfFiles; i++){ 
    //sofia------ create new params ------------------------------
    llvm::errs()<<"In for loop : "<<i<<"\n";
    std::unique_ptr<llvm::ToolOutputFile> output2;

    std::srand((unsigned int)time(NULL));
    int seed = rand()%1000000 + i;
    
    std::string mode = (i < NumOfFiles / 2) ? "stronger" : "weaker"; 

    std::string subfolder = InvFolder + "/" + mode + "_" + std::to_string(i);  

    // Need to change the name of .bc output    
    std::string newOutputFilename;
    if (!OutputFilename.empty()){ 
      //1) clam/invariants/core_0/transformed.bc --> transformed.bc
      newOutputFilename = OutputFilename.substr(InvFolder.size() + 1, OutputFilename.size()-(InvFolder.size() + 1));
      //2) transformed.bc --> transformed
      newOutputFilename = newOutputFilename.substr(0, newOutputFilename.size()-3);
      //3) transformed.bc --> transformed_stronger_0.bc
      newOutputFilename = newOutputFilename + "_" + mode + "_" + std::to_string(i) + ".bc" ;
      //4) transformed_stronger_0.bc --> clam/invariants/core_0/stronger_0/transformed_stronger_0.bc
      newOutputFilename = subfolder + "/" + newOutputFilename;
    }
    //-------------------------------------------------------------
    
    //sofia------ output ----------------------------------
    if (!OutputFilename.empty())
      output2 = std::make_unique<llvm::ToolOutputFile>( newOutputFilename.c_str(), error_code, llvm::sys::fs::F_None);

    if (error_code) {
      if (llvm::errs().has_colors())
        llvm::errs().changeColor(llvm::raw_ostream::RED);
      llvm::errs() << "error: Could not open " << OutputFilename << ": "
                  << error_code.message() << "\n";
      if (llvm::errs().has_colors())
        llvm::errs().resetColor();
      return 3;
    }
    //-----------------------------------------------------------------
    
    llvm::errs()<<"Starting pass manager "<<i<<"\n";
    //new Pass Manager--------------------------------------------
    llvm::legacy::PassManager pass_manager2;  
    
    pass_manager2.add(clam::createDeleteAssumePass());
    pass_manager2.add(clam::createDeleteCrabCommandPass());
    

    if (!DisableCrab && CrabOpt) {
      // post-processing of the bitcode using Crab invariants
      pass_manager2.add(clam::createOptimizerPass(&(*clam_analysis), seed, mode, subfolder));
      
      //pass_manager.add(llvm::createVerifierPass());
      //pass_manager2.add(clam::createPromoteAssertPass());
      
      //// Cleanup
      // -- simplify invariants added in the bitecode.  Sofia:
      // replaced by delete crab command pass
      //pass_manager2.add(llvm::createDeadCodeEliminationPass());

      //pass_manager2.add(clam::createInstCombine());   //problem
      // -- remove dead edges and blocks
      
      //pass_manager2.add(llvm::createCFGSimplificationPass());
      // -- remove global strings and values
      //pass_manager2.add(llvm::createGlobalDCEPass());

    }


   if (!OutputFilename.empty()) {
        pass_manager2.add(createBitcodeWriterPass(output2->os()));
    }

    //pass_manager2.run(*module.get()); 
    pass_manager2.run(*module.get()); 

    //if (!AsmOutputFilename.empty())
      //asmOutput->keep();

    if (!OutputFilename.empty()){ 
      output2->keep();
    }
  }  

  return 0;
}


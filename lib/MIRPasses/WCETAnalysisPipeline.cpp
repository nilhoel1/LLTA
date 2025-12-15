#include "MIRPasses/WCETAnalysisPipeline.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Scalar/LICM.h"
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Scalar/LoopRotation.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Scalar/SROA.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Analysis/AliasAnalysis.h"

// TODO: Include your custom pass header once it is available
// #include "LLTAAnalysisPass.h" 

using namespace llvm;



void llvm::runWCETAnalysisPipeline(Module &M) {
    // 1. Create the Pass Managers
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    // 2. Register the default alias analyses (BasicAA, TBAA, etc.)
    // This is CRITICAL. Without this, LICM is blind.
    PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // 3. Construct the "Preparation Pipeline"
    // We build a FunctionPassManager (FPM) to clean up every function.
    FunctionPassManager PrepFPM;

    // -- Step A: Basic Cleanup --
    // SROA breaks up aggregates (structs) into scalars so registers are used.
    PrepFPM.addPass(SROAPass(SROAOptions::ModifyCFG)); 
    // SimplifyCFG cleans up dead blocks that might confuse loop detection.
    PrepFPM.addPass(SimplifyCFGPass()); 

    // -- Step B: Loop Canonicalization (The "Secret Sauce" for SCEV) --
    // Loop Rotation puts loops in "Latch at bottom" form, which SCEV prefers.
    PrepFPM.addPass(createFunctionToLoopPassAdaptor(LoopRotatePass()));
    
    // LICM: Hoists invariants (requires AA). 
    // We run it BEFORE IndVars to ensure bounds variables are static.
    // Enable MemorySSA for LICM as required.
    PrepFPM.addPass(createFunctionToLoopPassAdaptor(LICMPass(LICMOptions()), /*UseMemorySSA=*/true));

    // IndVarSimplify: Turns complex iterators into simple 0..N counters.
    PrepFPM.addPass(createFunctionToLoopPassAdaptor(IndVarSimplifyPass()));

    // -- Step C: Run the Prep Pipeline --
    ModulePassManager MPM;
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(PrepFPM)));
    


    // -- Step D: Run Your LLTA Analysis --
    // Now that the IR is clean, we run your analysis.
    // Note: LLTAAnalysisPass should leverage the cached analyses (SCEV, etc.)
    // TODO: Enable this once LLTAAnalysisPass is available/linked
    // MPM.addPass(LLTAAnalysisPass());

    // 4. Execute
    MPM.run(M, MAM);
}

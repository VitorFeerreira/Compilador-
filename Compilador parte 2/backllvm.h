#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Verifier.h>

#include <llvm/Passes/PassBuilder.h>
#include "llvm/IR/LegacyPassManager.h"

//#include "llvm/Support/TargetRegistry.h"
#include "llvm/MC/TargetRegistry.h"
#include <llvm/Support/Host.h>
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

LLVMContext ctx;
static IRBuilder<> backend(ctx);

Module *module;
BasicBlock *main_stmts;
Function *main_func;
Function *current_func;

// auxiliary functions of stdc.c
Function *printfloat = NULL;

void print_llvm_ir() {

	InitializeAllTargetInfos();
	InitializeAllTargets();
	InitializeAllTargetMCs();
	InitializeAllAsmParsers();
	InitializeAllAsmPrinters();

	/*auto TargetTriple = "avr-atmel-none";
	std::string Error;
	auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
	auto CPU = "atmega328p";
	auto Features = "+avr5";*/
	auto TargetTriple = sys::getDefaultTargetTriple();
	std::string Error;
	auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
	auto CPU = "";
	auto Features = "";


	TargetOptions opt;
	auto RM = std::optional<Reloc::Model>();
	auto targetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

	module->setDataLayout(targetMachine->createDataLayout());
	module->setTargetTriple(TargetTriple);

	PassBuilder passBuilder(targetMachine);
	auto loopAnalysisManager = LoopAnalysisManager{};
	auto functionAnalysisManager = FunctionAnalysisManager{};
	auto cGSCCAnalysisManager = CGSCCAnalysisManager{};
	auto moduleAnalysisManager = ModuleAnalysisManager{};

	passBuilder.registerModuleAnalyses(moduleAnalysisManager);
	passBuilder.registerCGSCCAnalyses(cGSCCAnalysisManager);
	passBuilder.registerFunctionAnalyses(functionAnalysisManager);
	passBuilder.registerLoopAnalyses(loopAnalysisManager);
	passBuilder.crossRegisterProxies(
	    loopAnalysisManager, functionAnalysisManager, cGSCCAnalysisManager, moduleAnalysisManager);

	ModulePassManager modulePassManager;
	
	// without optimization
	modulePassManager = passBuilder.buildO0DefaultPipeline(OptimizationLevel::O0);
	
	// with O2 optimization
    //modulePassManager = passBuilder.buildPerModuleDefaultPipeline(OptimizationLevel::O2);

	modulePassManager.run(*module, moduleAnalysisManager);

	#define ENABLE_STDOUT
	#ifdef ENABLE_STDOUT
	std::string outfilename = "out.o";
	std::error_code ec;
	raw_fd_ostream dest(outfilename, ec);
	if (ec) {
		printf("Error writing to %s.\n", outfilename.c_str());
		exit(1);
	}
	legacy::PassManager pass_codegen;
	targetMachine->addPassesToEmitFile(pass_codegen, dest, nullptr, CGFT_ObjectFile);
	pass_codegen.run(*module);
	dest.flush();
	#endif

	// print IR to stdout
	module->print(outs(), nullptr);
}

void create_printfloat() {
	if (printfloat)
		return;
	std::vector<Type*> arg_types;
	arg_types.push_back(Type::getDoubleTy(ctx));
	FunctionType *ftype = FunctionType::get(Type::getVoidTy(ctx), arg_types, false);
	printfloat = Function::Create(ftype, Function::ExternalLinkage, "printfloat", module);
	printfloat->setCallingConv(CallingConv::C);
}

void setup_llvm() {

	module = new Module("llvm program", ctx);
	
	FunctionType *ft = FunctionType::get(Type::getInt16Ty(ctx), ArrayRef<Type*>(), false);
	main_func = Function::Create(ft, GlobalValue::ExternalLinkage, "main", module);

	main_stmts = BasicBlock::Create(ctx, "entry", main_func);
	current_func = main_func;

	backend.SetInsertPoint(main_stmts);

	// declare auxiliary functions from stdc.c
	create_printfloat();

}



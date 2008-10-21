//===--- Backend.cpp - Interface to LLVM backend technologies -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ASTConsumers.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/TranslationUnit.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/CodeGen/ModuleBuilder.h"
#include "llvm/Module.h"
#include "llvm/ModuleProvider.h"
#include "llvm/PassManager.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/CodeGen/RegAllocRegistry.h"
#include "llvm/CodeGen/SchedulerRegistry.h"
#include "llvm/CodeGen/ScheduleDAG.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Streams.h"
#include "llvm/Support/Compiler.h"
#include "llvm/System/Path.h"
#include "llvm/System/Program.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetMachineRegistry.h"
#include <fstream> // FIXME: Remove

using namespace clang;
using namespace llvm;

namespace {
  class VISIBILITY_HIDDEN BackendConsumer  : public ASTConsumer {
    BackendAction Action;
    const std::string &InputFile;
    std::string OutputFile;
    llvm::OwningPtr<CodeGenerator> Gen;
    
    llvm::Module *TheModule;
    llvm::TargetData *TheTargetData;
    llvm::raw_ostream *AsmOutStream;

    mutable FunctionPassManager *CodeGenPasses;
    mutable PassManager *PerModulePasses;
    mutable FunctionPassManager *PerFunctionPasses;

    FunctionPassManager *getCodeGenPasses() const;
    PassManager *getPerModulePasses() const;
    FunctionPassManager *getPerFunctionPasses() const;

    void CreatePasses();

    /// AddEmitPasses - Add passes necessary to emit assembly or LLVM
    /// IR.
    ///
    /// \arg Fast - Whether "fast" native compilation should be
    /// used. This implies local register allocation and fast
    /// instruction selection.
    /// \return True on success. On failure \arg Error will be set to
    /// a user readable error message.
    bool AddEmitPasses(bool Fast, std::string &Error);

    void EmitAssembly();
    
  public:  
    BackendConsumer(BackendAction action, Diagnostic &Diags, 
                    const LangOptions &Features,
                    const std::string& infile, const std::string& outfile,
                    bool GenerateDebugInfo)  :
      Action(action), 
      InputFile(infile), 
      OutputFile(outfile), 
      Gen(CreateLLVMCodeGen(Diags, Features, InputFile, GenerateDebugInfo)),
      TheModule(0), TheTargetData(0), AsmOutStream(0),
      CodeGenPasses(0), PerModulePasses(0), PerFunctionPasses(0) {}

    ~BackendConsumer() {
      // FIXME: Move out of destructor.
      EmitAssembly();

      delete AsmOutStream;
      delete TheTargetData;
      delete TheModule;
      delete CodeGenPasses;
      delete PerModulePasses;
      delete PerFunctionPasses;
    }

    virtual void InitializeTU(TranslationUnit& TU) {
      Gen->InitializeTU(TU);

      TheModule = Gen->GetModule();
      TheTargetData = 
        new llvm::TargetData(TU.getContext().Target.getTargetDescription());
    }
    
    virtual void HandleTopLevelDecl(Decl *D) {
      Gen->HandleTopLevelDecl(D);
    }
    
    virtual void HandleTranslationUnit(TranslationUnit& TU) {
      Gen->HandleTranslationUnit(TU);
    }
    
    virtual void HandleTagDeclDefinition(TagDecl *D) {
      Gen->HandleTagDeclDefinition(D);
    }
  };  
}

FunctionPassManager *BackendConsumer::getCodeGenPasses() const {
  if (!CodeGenPasses) {
    CodeGenPasses = 
      new FunctionPassManager(new ExistingModuleProvider(TheModule));
    CodeGenPasses->add(new TargetData(*TheTargetData));
  }

  return CodeGenPasses;
}

PassManager *BackendConsumer::getPerModulePasses() const {
  if (!PerModulePasses) {
    PerModulePasses = new PassManager();
    PerModulePasses->add(new TargetData(*TheTargetData));
  }

  return PerModulePasses;
}

FunctionPassManager *BackendConsumer::getPerFunctionPasses() const {
  if (!PerFunctionPasses) {
    PerFunctionPasses = 
      new FunctionPassManager(new ExistingModuleProvider(TheModule));
    PerFunctionPasses->add(new TargetData(*TheTargetData));
  }

  return PerFunctionPasses;
}

bool BackendConsumer::AddEmitPasses(bool Fast, std::string &Error) {
  // Create the TargetMachine for generating code.
  const TargetMachineRegistry::entry *TME = 
    TargetMachineRegistry::getClosestStaticTargetForModule(*TheModule, Error);
  if (!TME) {
    Error = std::string("Unable to get target machine: ") + Error;
    return false;
  }
      
  // FIXME: Support features?
  std::string FeatureStr;
  TargetMachine *TM = TME->CtorFn(*TheModule, FeatureStr);
  
  // Set register scheduler & allocation policy.
  RegisterScheduler::setDefault(createDefaultScheduler);
  RegisterRegAlloc::setDefault(Fast ? createLocalRegisterAllocator : 
                               createLinearScanRegisterAllocator);  

  // This is ridiculous.
  // FIXME: These aren't being release for now. I'm just going to fix
  // things to use raw_ostream instead.
  std::ostream *AsmStdOutStream = 0;
  OStream *AsmLLVMOutStream = 0;
  if (OutputFile == "-" || (InputFile == "-" && OutputFile.empty())) {
    AsmStdOutStream = llvm::cout.stream();
    AsmLLVMOutStream = &llvm::cout;
    AsmOutStream = new raw_stdout_ostream();
    sys::Program::ChangeStdoutToBinary();
  } else {
    if (OutputFile.empty()) {
      llvm::sys::Path Path(InputFile);
      Path.eraseSuffix();
      if (Action == Backend_EmitBC) {
        Path.appendSuffix("bc");
      } else if (Action == Backend_EmitLL) {
        Path.appendSuffix("ll");
      } else {
        Path.appendSuffix("s");
      }
      OutputFile = Path.toString();
    }

    // FIXME: raw_fd_ostream should specify its non-error condition
    // better.
    Error = "";
    AsmStdOutStream = new std::ofstream(OutputFile.c_str(),
                                        (std::ios_base::binary | 
                                         std::ios_base::out));
    AsmLLVMOutStream = new OStream(AsmStdOutStream);
    AsmOutStream = new raw_os_ostream(*AsmStdOutStream);
    if (!Error.empty())
      return false;
  }

  if (Action == Backend_EmitBC) {
    getPerModulePasses()->add(CreateBitcodeWriterPass(*AsmStdOutStream));
  } else if (Action == Backend_EmitLL) {
    getPerModulePasses()->add(createPrintModulePass(AsmLLVMOutStream));
  } else {
    // From llvm-gcc:
    // If there are passes we have to run on the entire module, we do codegen
    // as a separate "pass" after that happens.
    // FIXME: This is disabled right now until bugs can be worked out.  Reenable
    // this for fast -O0 compiles!
    FunctionPassManager *PM = getCodeGenPasses();
    
    // Normal mode, emit a .s file by running the code generator.
    // Note, this also adds codegenerator level optimization passes.
    switch (TM->addPassesToEmitFile(*PM, *AsmOutStream,
                                    TargetMachine::AssemblyFile, Fast)) {
    default:
    case FileModel::Error:
      Error = "Unable to interface with target machine!\n";
      return false;
    case FileModel::AsmFile:
      break;
    }
    
    if (TM->addPassesToEmitFileFinish(*CodeGenPasses, 0, Fast)) {
      Error = "Unable to interface with target machine!\n";
      return false;
    }
  }

  return true;
}

void BackendConsumer::CreatePasses() {
  
}

/// EmitAssembly - Handle interaction with LLVM backend to generate
/// actual machine code. 
void BackendConsumer::EmitAssembly() {
  // Silently ignore if we weren't initialized for some reason.
  if (!TheModule || !TheTargetData)
    return;

  bool Optimize = false; // FIXME

  // Make sure IR generation is happy with the module.
  // FIXME: Release this.
  Module *M = Gen->ReleaseModule();
  if (!M) {
    TheModule = 0;
    return;
  }

  assert(TheModule == M && "Unexpected module change during IR generation");

  CreatePasses();

  std::string Error;
  if (!AddEmitPasses(!Optimize, Error)) {
    // FIXME: Don't fail this way.
    llvm::cerr << "ERROR: " << Error << "\n";
    ::exit(1);
  }

  // Run passes. For now we do all passes at once, but eventually we
  // would like to have the option of streaming code generation.

  if (PerFunctionPasses) {
    PerFunctionPasses->doInitialization();
    for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I)
      if (!I->isDeclaration())
        PerFunctionPasses->run(*I);
    PerFunctionPasses->doFinalization();
  }
  
  if (PerModulePasses)
    PerModulePasses->run(*M);
  
  if (CodeGenPasses) {
    CodeGenPasses->doInitialization();
    for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I)
      if (!I->isDeclaration())
        CodeGenPasses->run(*I);
    CodeGenPasses->doFinalization();
  }
}

ASTConsumer *clang::CreateBackendConsumer(BackendAction Action,
                                          Diagnostic &Diags,
                                          const LangOptions &Features,
                                          const std::string& InFile,
                                          const std::string& OutFile,
                                          bool GenerateDebugInfo) {
  return new BackendConsumer(Action, Diags, Features, InFile, OutFile,
                             GenerateDebugInfo);  
}

//===--- CodeGenModule.h - Per-Module state for LLVM CodeGen ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the internal per-translation-unit state used for llvm translation.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_CODEGEN_CODEGENMODULE_H
#define CLANG_CODEGEN_CODEGENMODULE_H

#include "CodeGenTypes.h"
#include "clang/AST/Attr.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"

#include "CGBlocks.h"
#include "CGCall.h"

#include <list>

namespace llvm {
  class Module;
  class Constant;
  class Function;
  class GlobalValue;
  class TargetData;
  class FunctionType;
}

namespace clang {
  class ASTContext;
  class FunctionDecl;
  class IdentifierInfo;
  class ObjCMethodDecl;
  class ObjCImplementationDecl;
  class ObjCCategoryImplDecl;
  class ObjCProtocolDecl;
  class ObjCEncodeExpr;
  class BlockExpr;
  class Decl;
  class Expr;
  class Stmt;
  class StringLiteral;
  class NamedDecl;
  class ValueDecl;
  class VarDecl;
  struct LangOptions;
  class Diagnostic;
  class AnnotateAttr;

namespace CodeGen {

  class CodeGenFunction;
  class CGDebugInfo;
  class CGObjCRuntime;

/// CodeGenModule - This class organizes the cross-function state that is used
/// while generating LLVM code.
class CodeGenModule : public BlockModule {
  CodeGenModule(const CodeGenModule&);  // DO NOT IMPLEMENT
  void operator=(const CodeGenModule&); // DO NOT IMPLEMENT

  typedef std::vector< std::pair<llvm::Constant*, int> > CtorList;

  ASTContext &Context;
  const LangOptions &Features;
  llvm::Module &TheModule;
  const llvm::TargetData &TheTargetData;
  Diagnostic &Diags;
  CodeGenTypes Types;
  CGObjCRuntime* Runtime;
  CGDebugInfo* DebugInfo;

  llvm::Function *MemCpyFn;
  llvm::Function *MemMoveFn;
  llvm::Function *MemSetFn;

  /// RuntimeFunctions - List of runtime functions whose names must be protected
  /// from introducing conflicts. These functions should be created unnamed, we
  /// will name them and patch up conflicts when we release the module.
  std::vector< std::pair<llvm::Function*, std::string> > RuntimeFunctions;

  /// GlobalDeclMap - Mapping of decl names (represented as unique
  /// character pointers from either the identifier table or the set
  /// of mangled names) to global variables we have already
  /// emitted. Note that the entries in this map are the actual
  /// globals and therefore may not be of the same type as the decl,
  /// they should be bitcasted on retrieval. Also note that the
  /// globals are keyed on their source name, not the global name
  /// (which may change with attributes such as asm-labels).  This key
  /// to this map should be generated using getMangledName().
  llvm::DenseMap<const char*, llvm::GlobalValue*> GlobalDeclMap;

  /// \brief Contains the strings used for mangled names.
  ///
  /// FIXME: Eventually, this should map from the semantic/canonical
  /// declaration for each global entity to its mangled name (if it
  /// has one).
  llvm::StringSet<> MangledNames;

  /// Aliases - List of aliases in module. These cannot be emitted until all the
  /// code has been seen, as they reference things by name instead of directly
  /// and may reference forward.
  std::vector<const FunctionDecl*> Aliases;

  /// DeferredDecls - List of decls for which code generation has been
  /// deferred. When the translation unit has been fully processed we
  /// will lazily emit definitions for only the decls that were
  /// actually used.  This should contain only Function and Var decls,
  /// and only those which actually define something.
  std::list<const ValueDecl*> DeferredDecls;

  /// LLVMUsed - List of global values which are required to be
  /// present in the object file; bitcast to i8*. This is used for
  /// forcing visibility of symbols which may otherwise be optimized
  /// out.
  std::vector<llvm::Constant*> LLVMUsed;

  /// GlobalCtors - Store the list of global constructors and their respective
  /// priorities to be emitted when the translation unit is complete.
  CtorList GlobalCtors;

  /// GlobalDtors - Store the list of global destructors and their respective
  /// priorities to be emitted when the translation unit is complete.
  CtorList GlobalDtors;

  std::vector<llvm::Constant*> Annotations;

  llvm::StringMap<llvm::Constant*> CFConstantStringMap;
  llvm::StringMap<llvm::Constant*> ConstantStringMap;

  /// CFConstantStringClassRef - Cached reference to the class for constant
  /// strings. This value has type int * but is actually an Obj-C class pointer.
  llvm::Constant *CFConstantStringClassRef;

  std::vector<llvm::Value *> BuiltinFunctions;
public:
  CodeGenModule(ASTContext &C, const LangOptions &Features, llvm::Module &M,
                const llvm::TargetData &TD, Diagnostic &Diags,
                bool GenerateDebugInfo);

  ~CodeGenModule();

  /// Release - Finalize LLVM code generation.
  void Release();

  /// getObjCRuntime() - Return a reference to the configured
  /// Objective-C runtime.
  CGObjCRuntime &getObjCRuntime() {
    assert(Runtime && "No Objective-C runtime has been configured.");
    return *Runtime;
  }

  /// hasObjCRuntime() - Return true iff an Objective-C runtime has
  /// been configured.
  bool hasObjCRuntime() { return !!Runtime; }

  CGDebugInfo *getDebugInfo() { return DebugInfo; }
  ASTContext &getContext() const { return Context; }
  const LangOptions &getLangOptions() const { return Features; }
  llvm::Module &getModule() const { return TheModule; }
  CodeGenTypes &getTypes() { return Types; }
  Diagnostic &getDiags() const { return Diags; }
  const llvm::TargetData &getTargetData() const { return TheTargetData; }

  /// GetAddrOfGlobalVar - Return the llvm::Constant for the address of the
  /// given global variable.
  llvm::Constant *GetAddrOfGlobalVar(const VarDecl *D);

  /// GetAddrOfFunction - Return the llvm::Constant for the address of the given
  /// function.
  llvm::Constant *GetAddrOfFunction(const FunctionDecl *D);

  /// GetStringForStringLiteral - Return the appropriate bytes for a string
  /// literal, properly padded to match the literal type. If only the address of
  /// a constant is needed consider using GetAddrOfConstantStringLiteral.
  std::string GetStringForStringLiteral(const StringLiteral *E);

  /// GetAddrOfConstantCFString - Return a pointer to a constant CFString object
  /// for the given string.
  llvm::Constant *GetAddrOfConstantCFString(const std::string& str);

  /// GetAddrOfConstantStringFromLiteral - Return a pointer to a constant array
  /// for the given string literal.
  llvm::Constant *GetAddrOfConstantStringFromLiteral(const StringLiteral *S);

  /// GetAddrOfConstantStringFromObjCEncode - Return a pointer to a constant
  /// array for the given ObjCEncodeExpr node.
  llvm::Constant *GetAddrOfConstantStringFromObjCEncode(const ObjCEncodeExpr *);
  
  /// GetAddrOfConstantString - Returns a pointer to a character array
  /// containing the literal. This contents are exactly that of the given
  /// string, i.e. it will not be null terminated automatically; see
  /// GetAddrOfConstantCString. Note that whether the result is actually a
  /// pointer to an LLVM constant depends on Feature.WriteableStrings.
  ///
  /// The result has pointer to array type.
  ///
  /// \param GlobalName If provided, the name to use for the global
  /// (if one is created).
  llvm::Constant *GetAddrOfConstantString(const std::string& str,
                                          const char *GlobalName=0);

  /// GetAddrOfConstantCString - Returns a pointer to a character array
  /// containing the literal and a terminating '\0' character. The result has
  /// pointer to array type.
  ///
  /// \param GlobalName If provided, the name to use for the global (if one is
  /// created).
  llvm::Constant *GetAddrOfConstantCString(const std::string &str,
                                           const char *GlobalName=0);

  llvm::Constant *GetAddrOfGlobalBlock(const BlockExpr *BE, const char *);

  /// getBuiltinLibFunction - Given a builtin id for a function like
  /// "__builtin_fabsf", return a Function* for "fabsf".
  llvm::Value *getBuiltinLibFunction(unsigned BuiltinID);

  llvm::Function *getMemCpyFn();
  llvm::Function *getMemMoveFn();
  llvm::Function *getMemSetFn();
  llvm::Function *getIntrinsic(unsigned IID, const llvm::Type **Tys = 0,
                               unsigned NumTys = 0);

  /// EmitTopLevelDecl - Emit code for a single top level declaration.
  void EmitTopLevelDecl(Decl *D);

  /// AddUsedGlobal - Add a global which should be forced to be
  /// present in the object file; these are emitted to the llvm.used
  /// metadata global.
  void AddUsedGlobal(llvm::GlobalValue *GV);

  void AddAnnotation(llvm::Constant *C) { Annotations.push_back(C); }

  /// CreateRuntimeFunction - Create a new runtime function whose name must be
  /// protected from collisions.
  llvm::Function *CreateRuntimeFunction(const llvm::FunctionType *Ty,
                                        const std::string &Name);

  void UpdateCompletedType(const TagDecl *D);

  /// EmitConstantExpr - Try to emit the given expression as a
  /// constant; returns 0 if the expression cannot be emitted as a
  /// constant.
  llvm::Constant *EmitConstantExpr(const Expr *E, CodeGenFunction *CGF = 0);

  llvm::Constant *EmitAnnotateAttr(llvm::GlobalValue *GV,
                                   const AnnotateAttr *AA, unsigned LineNo);

  /// ErrorUnsupported - Print out an error that codegen doesn't support the
  /// specified stmt yet.
  /// \param OmitOnError - If true, then this error should only be emitted if no
  /// other errors have been reported.
  void ErrorUnsupported(const Stmt *S, const char *Type,
                        bool OmitOnError=false);

  /// ErrorUnsupported - Print out an error that codegen doesn't support the
  /// specified decl yet.
  /// \param OmitOnError - If true, then this error should only be emitted if no
  /// other errors have been reported.
  void ErrorUnsupported(const Decl *D, const char *Type,
                        bool OmitOnError=false);

  void SetMethodAttributes(const ObjCMethodDecl *MD,
                           llvm::Function *F);

  void SetFunctionAttributes(const Decl *D,
                             const CGFunctionInfo &Info,
                             llvm::Function *F);

  /// ReturnTypeUsesSret - Return true iff the given type uses 'sret' when used
  /// as a return type.
  bool ReturnTypeUsesSret(const CGFunctionInfo &FI);

  void ConstructAttributeList(const CGFunctionInfo &Info,
                              const Decl *TargetDecl,
                              AttributeListType &PAL);

  const char *getMangledName(const NamedDecl *ND);


private:
  /// SetGlobalValueAttributes - Set attributes for a global decl.
  void SetGlobalValueAttributes(const Decl *D, 
                                bool IsInternal,
                                bool IsInline,
                                llvm::GlobalValue *GV,
                                bool ForDefinition);
    
  /// SetFunctionAttributesForDefinition - Set function attributes specific to a
  /// function definition.
  /// \param D - The ObjCMethodDecl or FunctionDecl defining \arg F.
  void SetFunctionAttributesForDefinition(const Decl *D,
                                          llvm::Function *F);

  void SetFunctionAttributes(const FunctionDecl *FD,
                             llvm::Function *F);

  /// EmitGlobal - Emit code for a singal global function or var decl. Forward
  /// declarations are emitted lazily.
  void EmitGlobal(const ValueDecl *D);

  void EmitGlobalDefinition(const ValueDecl *D);

  /// EmitForwardFunctionDefinition - Create a new function for the
  /// given decl and set attributes as appropriate.
  ///
  /// \arg Ty - If non-null the LLVM function type to use for the
  /// decl; it is the callers responsibility to make sure this is
  /// compatible with the correct type.
  llvm::GlobalValue *EmitForwardFunctionDefinition(const FunctionDecl *D,
                                                   const llvm::Type *Ty);

  void EmitGlobalFunctionDefinition(const FunctionDecl *D);
  void EmitGlobalVarDefinition(const VarDecl *D);
  void EmitObjCPropertyImplementations(const ObjCImplementationDecl *D);

  // FIXME: Hardcoding priority here is gross.
  void AddGlobalCtor(llvm::Function * Ctor, int Priority=65535);
  void AddGlobalDtor(llvm::Function * Dtor, int Priority=65535);

  /// EmitCtorList - Generates a global array of functions and priorities using
  /// the given list and name. This array will have appending linkage and is
  /// suitable for use as a LLVM constructor or destructor array.
  void EmitCtorList(const CtorList &Fns, const char *GlobalName);

  void EmitAliases(void);
  void EmitAnnotations(void);

  /// EmitDeferred - Emit any needed decls for which code generation
  /// was deferred.
  void EmitDeferred(void);

  /// EmitLLVMUsed - Emit the llvm.used metadata used to force
  /// references to global which may otherwise be optimized out.
  void EmitLLVMUsed(void);

  void BindRuntimeFunctions();

  /// MayDeferGeneration - Determine if the given decl can be emitted
  /// lazily; this is only relevant for definitions. The given decl
  /// must be either a function or var decl.
  bool MayDeferGeneration(const ValueDecl *D);
};
}  // end namespace CodeGen
}  // end namespace clang

#endif

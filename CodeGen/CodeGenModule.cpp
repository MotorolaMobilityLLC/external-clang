//===--- CodeGenModule.cpp - Emit LLVM Code from ASTs for a Module --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This coordinates the per-module state used while generating code.
//
//===----------------------------------------------------------------------===//

#include "CodeGenModule.h"
#include "CodeGenFunction.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Intrinsics.h"
#include <algorithm>
using namespace clang;
using namespace CodeGen;


CodeGenModule::CodeGenModule(ASTContext &C, const LangOptions &LO,
                             llvm::Module &M, const llvm::TargetData &TD,
                             Diagnostic &diags)
  : Context(C), Features(LO), TheModule(M), TheTargetData(TD), Diags(diags),
    Types(C, M, TD), MemCpyFn(0), MemSetFn(0), CFConstantStringClassRef(0) {
  //TODO: Make this selectable at runtime
  Runtime = CreateObjCRuntime(M);
}

CodeGenModule::~CodeGenModule() {
  delete Runtime;
}

/// WarnUnsupported - Print out a warning that codegen doesn't support the
/// specified stmt yet.
void CodeGenModule::WarnUnsupported(const Stmt *S, const char *Type) {
  unsigned DiagID = getDiags().getCustomDiagID(Diagnostic::Warning, 
                                               "cannot codegen this %0 yet");
  SourceRange Range = S->getSourceRange();
  std::string Msg = Type;
  getDiags().Report(Context.getFullLoc(S->getLocStart()), DiagID,
                    &Msg, 1, &Range, 1);
}

/// WarnUnsupported - Print out a warning that codegen doesn't support the
/// specified decl yet.
void CodeGenModule::WarnUnsupported(const Decl *D, const char *Type) {
  unsigned DiagID = getDiags().getCustomDiagID(Diagnostic::Warning, 
                                               "cannot codegen this %0 yet");
  std::string Msg = Type;
  getDiags().Report(Context.getFullLoc(D->getLocation()), DiagID,
                    &Msg, 1);
}

/// ReplaceMapValuesWith - This is a really slow and bad function that
/// searches for any entries in GlobalDeclMap that point to OldVal, changing
/// them to point to NewVal.  This is badbadbad, FIXME!
void CodeGenModule::ReplaceMapValuesWith(llvm::Constant *OldVal,
                                         llvm::Constant *NewVal) {
  for (llvm::DenseMap<const Decl*, llvm::Constant*>::iterator 
       I = GlobalDeclMap.begin(), E = GlobalDeclMap.end(); I != E; ++I)
    if (I->second == OldVal) I->second = NewVal;
}


llvm::Constant *CodeGenModule::GetAddrOfFunctionDecl(const FunctionDecl *D,
                                                     bool isDefinition) {
  // See if it is already in the map.  If so, just return it.
  llvm::Constant *&Entry = GlobalDeclMap[D];
  if (Entry) return Entry;
  
  const llvm::Type *Ty = getTypes().ConvertType(D->getType());
  
  // Check to see if the function already exists.
  llvm::Function *F = getModule().getFunction(D->getName());
  const llvm::FunctionType *FTy = cast<llvm::FunctionType>(Ty);

  // If it doesn't already exist, just create and return an entry.
  if (F == 0) {
    // FIXME: param attributes for sext/zext etc.
    return Entry = new llvm::Function(FTy, llvm::Function::ExternalLinkage,
                                      D->getName(), &getModule());
  }
  
  // If the pointer type matches, just return it.
  llvm::Type *PFTy = llvm::PointerType::getUnqual(Ty);
  if (PFTy == F->getType()) return Entry = F;
    
  // If this isn't a definition, just return it casted to the right type.
  if (!isDefinition)
    return Entry = llvm::ConstantExpr::getBitCast(F, PFTy);
  
  // Otherwise, we have a definition after a prototype with the wrong type.
  // F is the Function* for the one with the wrong type, we must make a new
  // Function* and update everything that used F (a declaration) with the new
  // Function* (which will be a definition).
  //
  // This happens if there is a prototype for a function (e.g. "int f()") and
  // then a definition of a different type (e.g. "int f(int x)").  Start by
  // making a new function of the correct type, RAUW, then steal the name.
  llvm::Function *NewFn = new llvm::Function(FTy, 
                                             llvm::Function::ExternalLinkage,
                                             "", &getModule());
  NewFn->takeName(F);
  
  // Replace uses of F with the Function we will endow with a body.
  llvm::Constant *NewPtrForOldDecl = 
    llvm::ConstantExpr::getBitCast(NewFn, F->getType());
  F->replaceAllUsesWith(NewPtrForOldDecl);
  
  // FIXME: Update the globaldeclmap for the previous decl of this name.  We
  // really want a way to walk all of these, but we don't have it yet.  This
  // is incredibly slow!
  ReplaceMapValuesWith(F, NewPtrForOldDecl);
  
  // Ok, delete the old function now, which is dead.
  assert(F->isDeclaration() && "Shouldn't replace non-declaration");
  F->eraseFromParent();

  // Return the new function which has the right type.
  return Entry = NewFn;
}

static bool IsZeroElementArray(const llvm::Type *Ty) {
  if (const llvm::ArrayType *ATy = dyn_cast<llvm::ArrayType>(Ty))
    return ATy->getNumElements() == 0;
  return false;
}

llvm::Constant *CodeGenModule::GetAddrOfGlobalVar(const VarDecl *D,
                                                  bool isDefinition) {
  assert(D->hasGlobalStorage() && "Not a global variable");
  
  // See if it is already in the map.
  llvm::Constant *&Entry = GlobalDeclMap[D];
  if (Entry) return Entry;
  
  QualType ASTTy = D->getType();
  const llvm::Type *Ty = getTypes().ConvertTypeForMem(ASTTy);

  // Check to see if the global already exists.
  llvm::GlobalVariable *GV = getModule().getGlobalVariable(D->getName(), true);

  // If it doesn't already exist, just create and return an entry.
  if (GV == 0) {
    return Entry = new llvm::GlobalVariable(Ty, false, 
                                            llvm::GlobalValue::ExternalLinkage,
                                            0, D->getName(), &getModule(), 0,
                                            ASTTy.getAddressSpace());
  }
  
  // If the pointer type matches, just return it.
  llvm::Type *PTy = llvm::PointerType::getUnqual(Ty);
  if (PTy == GV->getType()) return Entry = GV;
  
  // If this isn't a definition, just return it casted to the right type.
  if (!isDefinition)
    return Entry = llvm::ConstantExpr::getBitCast(GV, PTy);
  
  
  // Otherwise, we have a definition after a prototype with the wrong type.
  // GV is the GlobalVariable* for the one with the wrong type, we must make a
  /// new GlobalVariable* and update everything that used GV (a declaration)
  // with the new GlobalVariable* (which will be a definition).
  //
  // This happens if there is a prototype for a global (e.g. "extern int x[];")
  // and then a definition of a different type (e.g. "int x[10];").  Start by
  // making a new global of the correct type, RAUW, then steal the name.
  llvm::GlobalVariable *NewGV = 
    new llvm::GlobalVariable(Ty, false, llvm::GlobalValue::ExternalLinkage,
                             0, D->getName(), &getModule(), 0,
                             ASTTy.getAddressSpace());
  NewGV->takeName(GV);
  
  // Replace uses of GV with the globalvalue we will endow with a body.
  llvm::Constant *NewPtrForOldDecl = 
    llvm::ConstantExpr::getBitCast(NewGV, GV->getType());
  GV->replaceAllUsesWith(NewPtrForOldDecl);
  
  // FIXME: Update the globaldeclmap for the previous decl of this name.  We
  // really want a way to walk all of these, but we don't have it yet.  This
  // is incredibly slow!
  ReplaceMapValuesWith(GV, NewPtrForOldDecl);
  
  // Verify that GV was a declaration or something like x[] which turns into
  // [0 x type].
  assert((GV->isDeclaration() || 
          IsZeroElementArray(GV->getType()->getElementType())) &&
         "Shouldn't replace non-declaration");
         
  // Ok, delete the old global now, which is dead.
  GV->eraseFromParent();
  
  // Return the new global which has the right type.
  return Entry = NewGV;
}


void CodeGenModule::EmitFunction(const FunctionDecl *FD) {
  // If this is not a prototype, emit the body.
  if (FD->getBody())
    CodeGenFunction(*this).GenerateCode(FD);
}

llvm::Constant *CodeGenModule::EmitGlobalInit(const Expr *Expr) {
  return EmitConstantExpr(Expr);
}

void CodeGenModule::EmitGlobalVar(const FileVarDecl *D) {
  // If this is just a forward declaration of the variable, don't emit it now,
  // allow it to be emitted lazily on its first use.
  if (D->getStorageClass() == VarDecl::Extern && D->getInit() == 0)
    return;
  
  // Get the global, forcing it to be a direct reference.
  llvm::GlobalVariable *GV = 
    cast<llvm::GlobalVariable>(GetAddrOfGlobalVar(D, true));
  
  // Convert the initializer, or use zero if appropriate.
  llvm::Constant *Init = 0;
  if (D->getInit() == 0) {
    Init = llvm::Constant::getNullValue(GV->getType()->getElementType());
  } else if (D->getType()->isIntegerType()) {
    llvm::APSInt Value(static_cast<uint32_t>(
      getContext().getTypeSize(D->getInit()->getType(), SourceLocation())));
    if (D->getInit()->isIntegerConstantExpr(Value, Context))
      Init = llvm::ConstantInt::get(Value);
  }

  if (!Init)
    Init = EmitGlobalInit(D->getInit());

  assert(GV->getType()->getElementType() == Init->getType() &&
         "Initializer codegen type mismatch!");
  GV->setInitializer(Init);

  if (const VisibilityAttr *attr = D->getAttr<VisibilityAttr>())
    GV->setVisibility(attr->getVisibility());
  // FIXME: else handle -fvisibility
  
  // Set the llvm linkage type as appropriate.
  if (D->getAttr<DLLImportAttr>())
    GV->setLinkage(llvm::Function::DLLImportLinkage);
  else if (D->getAttr<DLLExportAttr>())
    GV->setLinkage(llvm::Function::DLLExportLinkage);
  else if (D->getAttr<WeakAttr>()) {
    GV->setLinkage(llvm::GlobalVariable::WeakLinkage);

  } else {
    // FIXME: This isn't right.  This should handle common linkage and other
    // stuff.
    switch (D->getStorageClass()) {
    case VarDecl::Auto:
    case VarDecl::Register:
      assert(0 && "Can't have auto or register globals");
    case VarDecl::None:
      if (!D->getInit())
        GV->setLinkage(llvm::GlobalVariable::WeakLinkage);
      break;
    case VarDecl::Extern:
    case VarDecl::PrivateExtern:
      // todo: common
      break;
    case VarDecl::Static:
      GV->setLinkage(llvm::GlobalVariable::InternalLinkage);
      break;
    }
  }
}

/// EmitGlobalVarDeclarator - Emit all the global vars attached to the specified
/// declarator chain.
void CodeGenModule::EmitGlobalVarDeclarator(const FileVarDecl *D) {
  for (; D; D = cast_or_null<FileVarDecl>(D->getNextDeclarator()))
    EmitGlobalVar(D);
}

void CodeGenModule::UpdateCompletedType(const TagDecl *TD) {
  // Make sure that this type is translated.
  Types.UpdateCompletedType(TD);
}


/// getBuiltinLibFunction
llvm::Function *CodeGenModule::getBuiltinLibFunction(unsigned BuiltinID) {
  if (BuiltinID > BuiltinFunctions.size())
    BuiltinFunctions.resize(BuiltinID);
  
  // Cache looked up functions.  Since builtin id #0 is invalid we don't reserve
  // a slot for it.
  assert(BuiltinID && "Invalid Builtin ID");
  llvm::Function *&FunctionSlot = BuiltinFunctions[BuiltinID-1];
  if (FunctionSlot)
    return FunctionSlot;
  
  assert(Context.BuiltinInfo.isLibFunction(BuiltinID) && "isn't a lib fn");
  
  // Get the name, skip over the __builtin_ prefix.
  const char *Name = Context.BuiltinInfo.GetName(BuiltinID)+10;
  
  // Get the type for the builtin.
  QualType Type = Context.BuiltinInfo.GetBuiltinType(BuiltinID, Context);
  const llvm::FunctionType *Ty = 
    cast<llvm::FunctionType>(getTypes().ConvertType(Type));

  // FIXME: This has a serious problem with code like this:
  //  void abs() {}
  //    ... __builtin_abs(x);
  // The two versions of abs will collide.  The fix is for the builtin to win,
  // and for the existing one to be turned into a constantexpr cast of the
  // builtin.  In the case where the existing one is a static function, it
  // should just be renamed.
  if (llvm::Function *Existing = getModule().getFunction(Name)) {
    if (Existing->getFunctionType() == Ty && Existing->hasExternalLinkage())
      return FunctionSlot = Existing;
    assert(Existing == 0 && "FIXME: Name collision");
  }

  // FIXME: param attributes for sext/zext etc.
  return FunctionSlot = new llvm::Function(Ty, llvm::Function::ExternalLinkage,
                                           Name, &getModule());
}

llvm::Function *CodeGenModule::getIntrinsic(unsigned IID,const llvm::Type **Tys,
                                            unsigned NumTys) {
  return llvm::Intrinsic::getDeclaration(&getModule(),
                                         (llvm::Intrinsic::ID)IID, Tys, NumTys);
}

llvm::Function *CodeGenModule::getMemCpyFn() {
  if (MemCpyFn) return MemCpyFn;
  llvm::Intrinsic::ID IID;
  uint64_t Size; unsigned Align;
  Context.Target.getPointerInfo(Size, Align, FullSourceLoc());
  switch (Size) {
  default: assert(0 && "Unknown ptr width");
  case 32: IID = llvm::Intrinsic::memcpy_i32; break;
  case 64: IID = llvm::Intrinsic::memcpy_i64; break;
  }
  return MemCpyFn = getIntrinsic(IID);
}

llvm::Function *CodeGenModule::getMemSetFn() {
  if (MemSetFn) return MemSetFn;
  llvm::Intrinsic::ID IID;
  uint64_t Size; unsigned Align;
  Context.Target.getPointerInfo(Size, Align, FullSourceLoc());
  switch (Size) {
  default: assert(0 && "Unknown ptr width");
  case 32: IID = llvm::Intrinsic::memset_i32; break;
  case 64: IID = llvm::Intrinsic::memset_i64; break;
  }
  return MemSetFn = getIntrinsic(IID);
}

llvm::Constant *CodeGenModule::
GetAddrOfConstantCFString(const std::string &str) {
  llvm::StringMapEntry<llvm::Constant *> &Entry = 
    CFConstantStringMap.GetOrCreateValue(&str[0], &str[str.length()]);
  
  if (Entry.getValue())
    return Entry.getValue();
  
  std::vector<llvm::Constant*> Fields;
  
  if (!CFConstantStringClassRef) {
    const llvm::Type *Ty = getTypes().ConvertType(getContext().IntTy);
    Ty = llvm::ArrayType::get(Ty, 0);
  
    CFConstantStringClassRef = 
      new llvm::GlobalVariable(Ty, false,
                               llvm::GlobalVariable::ExternalLinkage, 0, 
                               "__CFConstantStringClassReference", 
                               &getModule());
  }
  
  // Class pointer.
  llvm::Constant *Zero = llvm::Constant::getNullValue(llvm::Type::Int32Ty);
  llvm::Constant *Zeros[] = { Zero, Zero };
  llvm::Constant *C = 
    llvm::ConstantExpr::getGetElementPtr(CFConstantStringClassRef, Zeros, 2);
  Fields.push_back(C);
  
  // Flags.
  const llvm::Type *Ty = getTypes().ConvertType(getContext().IntTy);
  Fields.push_back(llvm::ConstantInt::get(Ty, 1992));
    
  // String pointer.
  C = llvm::ConstantArray::get(str);
  C = new llvm::GlobalVariable(C->getType(), true, 
                               llvm::GlobalValue::InternalLinkage,
                               C, ".str", &getModule());
  
  C = llvm::ConstantExpr::getGetElementPtr(C, Zeros, 2);
  Fields.push_back(C);
  
  // String length.
  Ty = getTypes().ConvertType(getContext().LongTy);
  Fields.push_back(llvm::ConstantInt::get(Ty, str.length()));
  
  // The struct.
  Ty = getTypes().ConvertType(getContext().getCFConstantStringType());
  C = llvm::ConstantStruct::get(cast<llvm::StructType>(Ty), Fields);
  llvm::GlobalVariable *GV = 
    new llvm::GlobalVariable(C->getType(), true, 
                             llvm::GlobalVariable::InternalLinkage, 
                             C, "", &getModule());
  GV->setSection("__DATA,__cfstring");
  Entry.setValue(GV);
  return GV;
}

/// GenerateWritableString -- Creates storage for a string literal.
static llvm::Constant *GenerateStringLiteral(const std::string &str, 
                                             bool constant,
                                             CodeGenModule &CGM) {
  // Create Constant for this string literal
  llvm::Constant *C=llvm::ConstantArray::get(str);
  
  // Create a global variable for this string
  C = new llvm::GlobalVariable(C->getType(), constant, 
                               llvm::GlobalValue::InternalLinkage,
                               C, ".str", &CGM.getModule());
  return C;
}

/// CodeGenModule::GetAddrOfConstantString -- returns a pointer to the character
/// array containing the literal.  The result is pointer to array type.
llvm::Constant *CodeGenModule::GetAddrOfConstantString(const std::string &str) {
  // Don't share any string literals if writable-strings is turned on.
  if (Features.WritableStrings)
    return GenerateStringLiteral(str, false, *this);
  
  llvm::StringMapEntry<llvm::Constant *> &Entry = 
  ConstantStringMap.GetOrCreateValue(&str[0], &str[str.length()]);

  if (Entry.getValue())
      return Entry.getValue();

  // Create a global variable for this.
  llvm::Constant *C = GenerateStringLiteral(str, true, *this);
  Entry.setValue(C);
  return C;
}

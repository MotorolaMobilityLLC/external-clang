//===--- CGDecl.cpp - Emit LLVM Code for declarations ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This contains code dealing with C++ code generation.
//
//===----------------------------------------------------------------------===//

// We might split this into multiple files if it gets too unwieldy

#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "Mangle.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/StmtCXX.h"
#include "llvm/ADT/StringExtras.h"
using namespace clang;
using namespace CodeGen;

void
CodeGenFunction::EmitCXXGlobalDtorRegistration(llvm::Constant *DtorFn,
                                               llvm::Constant *DeclPtr) {
  const llvm::Type *Int8PtrTy = 
    llvm::Type::getInt8Ty(VMContext)->getPointerTo();

  std::vector<const llvm::Type *> Params;
  Params.push_back(Int8PtrTy);

  // Get the destructor function type
  const llvm::Type *DtorFnTy =
    llvm::FunctionType::get(llvm::Type::getVoidTy(VMContext), Params, false);
  DtorFnTy = llvm::PointerType::getUnqual(DtorFnTy);

  Params.clear();
  Params.push_back(DtorFnTy);
  Params.push_back(Int8PtrTy);
  Params.push_back(Int8PtrTy);

  // Get the __cxa_atexit function type
  // extern "C" int __cxa_atexit ( void (*f)(void *), void *p, void *d );
  const llvm::FunctionType *AtExitFnTy =
    llvm::FunctionType::get(ConvertType(getContext().IntTy), Params, false);

  llvm::Constant *AtExitFn = CGM.CreateRuntimeFunction(AtExitFnTy,
                                                       "__cxa_atexit");

  llvm::Constant *Handle = CGM.CreateRuntimeVariable(Int8PtrTy,
                                                     "__dso_handle");
  llvm::Value *Args[3] = { llvm::ConstantExpr::getBitCast(DtorFn, DtorFnTy),
                           llvm::ConstantExpr::getBitCast(DeclPtr, Int8PtrTy),
                           llvm::ConstantExpr::getBitCast(Handle, Int8PtrTy) };
  Builder.CreateCall(AtExitFn, &Args[0], llvm::array_endof(Args));
}

void CodeGenFunction::EmitCXXGlobalVarDeclInit(const VarDecl &D,
                                               llvm::Constant *DeclPtr) {
  assert(D.hasGlobalStorage() &&
         "VarDecl must have global storage!");

  const Expr *Init = D.getInit();
  QualType T = D.getType();
  bool isVolatile = getContext().getCanonicalType(T).isVolatileQualified();

  if (T->isReferenceType()) {
    ErrorUnsupported(Init, "global variable that binds to a reference");
  } else if (!hasAggregateLLVMType(T)) {
    llvm::Value *V = EmitScalarExpr(Init);
    EmitStoreOfScalar(V, DeclPtr, isVolatile, T);
  } else if (T->isAnyComplexType()) {
    EmitComplexExprIntoAddr(Init, DeclPtr, isVolatile);
  } else {
    EmitAggExpr(Init, DeclPtr, isVolatile);
    // Avoid generating destructor(s) for initialized objects. 
    if (!isa<CXXConstructExpr>(Init))
      return;
    const ConstantArrayType *Array = getContext().getAsConstantArrayType(T);
    if (Array)
      T = getContext().getBaseElementType(Array);
    
    if (const RecordType *RT = T->getAs<RecordType>()) {
      CXXRecordDecl *RD = cast<CXXRecordDecl>(RT->getDecl());
      if (!RD->hasTrivialDestructor()) {
        llvm::Constant *DtorFn;
        if (Array) {
          DtorFn = CodeGenFunction(CGM).GenerateCXXAggrDestructorHelper(
                                                RD->getDestructor(getContext()), 
                                                Array, DeclPtr);
          DeclPtr = 
            llvm::Constant::getNullValue(llvm::Type::getInt8PtrTy(VMContext));
        }
        else
          DtorFn = CGM.GetAddrOfCXXDestructor(RD->getDestructor(getContext()), 
                                              Dtor_Complete);                                
        EmitCXXGlobalDtorRegistration(DtorFn, DeclPtr);
      }
    }
  }
}

void
CodeGenModule::EmitCXXGlobalInitFunc() {
  if (CXXGlobalInits.empty())
    return;

  const llvm::FunctionType *FTy
    = llvm::FunctionType::get(llvm::Type::getVoidTy(VMContext),
                              false);

  // Create our global initialization function.
  // FIXME: Should this be tweakable by targets?
  llvm::Function *Fn =
    llvm::Function::Create(FTy, llvm::GlobalValue::InternalLinkage,
                           "__cxx_global_initialization", &TheModule);

  CodeGenFunction(*this).GenerateCXXGlobalInitFunc(Fn,
                                                   &CXXGlobalInits[0],
                                                   CXXGlobalInits.size());
  AddGlobalCtor(Fn);
}

void CodeGenFunction::GenerateCXXGlobalInitFunc(llvm::Function *Fn,
                                                const VarDecl **Decls,
                                                unsigned NumDecls) {
  StartFunction(GlobalDecl(), getContext().VoidTy, Fn, FunctionArgList(),
                SourceLocation());

  for (unsigned i = 0; i != NumDecls; ++i) {
    const VarDecl *D = Decls[i];

    llvm::Constant *DeclPtr = CGM.GetAddrOfGlobalVar(D);
    EmitCXXGlobalVarDeclInit(*D, DeclPtr);
  }
  FinishFunction();
}

void
CodeGenFunction::EmitStaticCXXBlockVarDeclInit(const VarDecl &D,
                                               llvm::GlobalVariable *GV) {
  // FIXME: This should use __cxa_guard_{acquire,release}?

  assert(!getContext().getLangOptions().ThreadsafeStatics &&
         "thread safe statics are currently not supported!");

  llvm::SmallString<256> GuardVName;
  CGM.getMangleContext().mangleGuardVariable(&D, GuardVName);

  // Create the guard variable.
  llvm::GlobalValue *GuardV =
    new llvm::GlobalVariable(CGM.getModule(), llvm::Type::getInt64Ty(VMContext),
                             false, GV->getLinkage(),
                llvm::Constant::getNullValue(llvm::Type::getInt64Ty(VMContext)),
                             GuardVName.str());

  // Load the first byte of the guard variable.
  const llvm::Type *PtrTy
    = llvm::PointerType::get(llvm::Type::getInt8Ty(VMContext), 0);
  llvm::Value *V = Builder.CreateLoad(Builder.CreateBitCast(GuardV, PtrTy),
                                      "tmp");

  // Compare it against 0.
  llvm::Value *nullValue
    = llvm::Constant::getNullValue(llvm::Type::getInt8Ty(VMContext));
  llvm::Value *ICmp = Builder.CreateICmpEQ(V, nullValue , "tobool");

  llvm::BasicBlock *InitBlock = createBasicBlock("init");
  llvm::BasicBlock *EndBlock = createBasicBlock("init.end");

  // If the guard variable is 0, jump to the initializer code.
  Builder.CreateCondBr(ICmp, InitBlock, EndBlock);

  EmitBlock(InitBlock);

  EmitCXXGlobalVarDeclInit(D, GV);

  Builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(VMContext),
                                             1),
                      Builder.CreateBitCast(GuardV, PtrTy));

  EmitBlock(EndBlock);
}

RValue CodeGenFunction::EmitCXXMemberCall(const CXXMethodDecl *MD,
                                          llvm::Value *Callee,
                                          llvm::Value *This,
                                          CallExpr::const_arg_iterator ArgBeg,
                                          CallExpr::const_arg_iterator ArgEnd) {
  assert(MD->isInstance() &&
         "Trying to emit a member call expr on a static method!");

  // A call to a trivial destructor requires no code generation.
  if (const CXXDestructorDecl *Destructor = dyn_cast<CXXDestructorDecl>(MD))
    if (Destructor->isTrivial())
      return RValue::get(0);

  const FunctionProtoType *FPT = MD->getType()->getAs<FunctionProtoType>();

  CallArgList Args;

  // Push the this ptr.
  Args.push_back(std::make_pair(RValue::get(This),
                                MD->getThisType(getContext())));

  // And the rest of the call args
  EmitCallArgs(Args, FPT, ArgBeg, ArgEnd);

  QualType ResultType = MD->getType()->getAs<FunctionType>()->getResultType();
  return EmitCall(CGM.getTypes().getFunctionInfo(ResultType, Args),
                  Callee, Args, MD);
}

/// canDevirtualizeMemberFunctionCalls - Checks whether virtual calls on given
/// expr can be devirtualized.
static bool canDevirtualizeMemberFunctionCalls(const Expr *Base) {
  if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(Base)) {
    if (const VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
      // This is a record decl. We know the type and can devirtualize it.
      return VD->getType()->isRecordType();
    }
    
    return false;
  }
  
  // We can always devirtualize calls on temporary object expressions.
  if (isa<CXXTemporaryObjectExpr>(Base))
    return true;
  
  // And calls on bound temporaries.
  if (isa<CXXBindTemporaryExpr>(Base))
    return true;
  
  // Check if this is a call expr that returns a record type.
  if (const CallExpr *CE = dyn_cast<CallExpr>(Base))
    return CE->getCallReturnType()->isRecordType();
  
  // We can't devirtualize the call.
  return false;
}

RValue CodeGenFunction::EmitCXXMemberCallExpr(const CXXMemberCallExpr *CE) {
  if (isa<BinaryOperator>(CE->getCallee())) 
    return EmitCXXMemberPointerCallExpr(CE);
      
  const MemberExpr *ME = cast<MemberExpr>(CE->getCallee());
  const CXXMethodDecl *MD = cast<CXXMethodDecl>(ME->getMemberDecl());

  if (MD->isStatic()) {
    // The method is static, emit it as we would a regular call.
    llvm::Value *Callee = CGM.GetAddrOfFunction(MD);
    return EmitCall(Callee, getContext().getPointerType(MD->getType()),
                    CE->arg_begin(), CE->arg_end(), 0);
    
  }
  
  const FunctionProtoType *FPT = MD->getType()->getAs<FunctionProtoType>();

  const llvm::Type *Ty =
    CGM.getTypes().GetFunctionType(CGM.getTypes().getFunctionInfo(MD),
                                   FPT->isVariadic());
  llvm::Value *This;

  if (ME->isArrow())
    This = EmitScalarExpr(ME->getBase());
  else {
    LValue BaseLV = EmitLValue(ME->getBase());
    This = BaseLV.getAddress();
  }

  // C++ [class.virtual]p12:
  //   Explicit qualification with the scope operator (5.1) suppresses the
  //   virtual call mechanism.
  //
  // We also don't emit a virtual call if the base expression has a record type
  // because then we know what the type is.
  llvm::Value *Callee;
  if (const CXXDestructorDecl *Destructor
             = dyn_cast<CXXDestructorDecl>(MD)) {
    if (MD->isVirtual() && !ME->hasQualifier() && 
        !canDevirtualizeMemberFunctionCalls(ME->getBase())) {
      Callee = BuildVirtualCall(Destructor, Dtor_Complete, This, Ty); 
    } else {
      Callee = CGM.GetAddrOfFunction(GlobalDecl(Destructor, Dtor_Complete), Ty);
    }
  } else if (MD->isVirtual() && !ME->hasQualifier() && 
             !canDevirtualizeMemberFunctionCalls(ME->getBase())) {
    Callee = BuildVirtualCall(MD, This, Ty); 
  } else {
    Callee = CGM.GetAddrOfFunction(MD, Ty);
  }

  return EmitCXXMemberCall(MD, Callee, This,
                           CE->arg_begin(), CE->arg_end());
}

RValue
CodeGenFunction::EmitCXXMemberPointerCallExpr(const CXXMemberCallExpr *E) {
  const BinaryOperator *BO = cast<BinaryOperator>(E->getCallee());
  const Expr *BaseExpr = BO->getLHS();
  const Expr *MemFnExpr = BO->getRHS();
  
  const MemberPointerType *MPT = 
    MemFnExpr->getType()->getAs<MemberPointerType>();
  const FunctionProtoType *FPT = 
    MPT->getPointeeType()->getAs<FunctionProtoType>();
  const CXXRecordDecl *RD = 
    cast<CXXRecordDecl>(MPT->getClass()->getAs<RecordType>()->getDecl());

  const llvm::FunctionType *FTy = 
    CGM.getTypes().GetFunctionType(CGM.getTypes().getFunctionInfo(RD, FPT),
                                   FPT->isVariadic());

  const llvm::Type *Int8PtrTy = 
    llvm::Type::getInt8Ty(VMContext)->getPointerTo();

  // Get the member function pointer.
  llvm::Value *MemFnPtr = 
    CreateTempAlloca(ConvertType(MemFnExpr->getType()), "mem.fn");
  EmitAggExpr(MemFnExpr, MemFnPtr, /*VolatileDest=*/false);

  // Emit the 'this' pointer.
  llvm::Value *This;
  
  if (BO->getOpcode() == BinaryOperator::PtrMemI)
    This = EmitScalarExpr(BaseExpr);
  else 
    This = EmitLValue(BaseExpr).getAddress();
  
  // Adjust it.
  llvm::Value *Adj = Builder.CreateStructGEP(MemFnPtr, 1);
  Adj = Builder.CreateLoad(Adj, "mem.fn.adj");
  
  llvm::Value *Ptr = Builder.CreateBitCast(This, Int8PtrTy, "ptr");
  Ptr = Builder.CreateGEP(Ptr, Adj, "adj");
  
  This = Builder.CreateBitCast(Ptr, This->getType(), "this");
  
  llvm::Value *FnPtr = Builder.CreateStructGEP(MemFnPtr, 0, "mem.fn.ptr");
  
  const llvm::Type *PtrDiffTy = ConvertType(getContext().getPointerDiffType());

  llvm::Value *FnAsInt = Builder.CreateLoad(FnPtr, "fn");
  
  // If the LSB in the function pointer is 1, the function pointer points to
  // a virtual function.
  llvm::Value *IsVirtual 
    = Builder.CreateAnd(FnAsInt, llvm::ConstantInt::get(PtrDiffTy, 1),
                        "and");
  
  IsVirtual = Builder.CreateTrunc(IsVirtual,
                                  llvm::Type::getInt1Ty(VMContext));
  
  llvm::BasicBlock *FnVirtual = createBasicBlock("fn.virtual");
  llvm::BasicBlock *FnNonVirtual = createBasicBlock("fn.nonvirtual");
  llvm::BasicBlock *FnEnd = createBasicBlock("fn.end");
  
  Builder.CreateCondBr(IsVirtual, FnVirtual, FnNonVirtual);
  EmitBlock(FnVirtual);
  
  const llvm::Type *VTableTy = 
    FTy->getPointerTo()->getPointerTo()->getPointerTo();

  llvm::Value *VTable = Builder.CreateBitCast(This, VTableTy);
  VTable = Builder.CreateLoad(VTable);
  
  VTable = Builder.CreateGEP(VTable, FnAsInt, "fn");
  
  // Since the function pointer is 1 plus the virtual table offset, we
  // subtract 1 by using a GEP.
  VTable = Builder.CreateConstGEP1_64(VTable, (uint64_t)-1);
  
  llvm::Value *VirtualFn = Builder.CreateLoad(VTable, "virtualfn");
  
  EmitBranch(FnEnd);
  EmitBlock(FnNonVirtual);
  
  // If the function is not virtual, just load the pointer.
  llvm::Value *NonVirtualFn = Builder.CreateLoad(FnPtr, "fn");
  NonVirtualFn = Builder.CreateIntToPtr(NonVirtualFn, FTy->getPointerTo());
  
  EmitBlock(FnEnd);

  llvm::PHINode *Callee = Builder.CreatePHI(FTy->getPointerTo());
  Callee->reserveOperandSpace(2);
  Callee->addIncoming(VirtualFn, FnVirtual);
  Callee->addIncoming(NonVirtualFn, FnNonVirtual);

  CallArgList Args;

  QualType ThisType = 
    getContext().getPointerType(getContext().getTagDeclType(RD));

  // Push the this ptr.
  Args.push_back(std::make_pair(RValue::get(This), ThisType));
  
  // And the rest of the call args
  EmitCallArgs(Args, FPT, E->arg_begin(), E->arg_end());
  QualType ResultType = BO->getType()->getAs<FunctionType>()->getResultType();
  return EmitCall(CGM.getTypes().getFunctionInfo(ResultType, Args),
                  Callee, Args, 0);
}

RValue
CodeGenFunction::EmitCXXOperatorMemberCallExpr(const CXXOperatorCallExpr *E,
                                               const CXXMethodDecl *MD) {
  assert(MD->isInstance() &&
         "Trying to emit a member call expr on a static method!");

  if (MD->isCopyAssignment()) {
    const CXXRecordDecl *ClassDecl = cast<CXXRecordDecl>(MD->getDeclContext());
    if (ClassDecl->hasTrivialCopyAssignment()) {
      assert(!ClassDecl->hasUserDeclaredCopyAssignment() &&
             "EmitCXXOperatorMemberCallExpr - user declared copy assignment");
      llvm::Value *This = EmitLValue(E->getArg(0)).getAddress();
      llvm::Value *Src = EmitLValue(E->getArg(1)).getAddress();
      QualType Ty = E->getType();
      EmitAggregateCopy(This, Src, Ty);
      return RValue::get(This);
    }
  }

  const FunctionProtoType *FPT = MD->getType()->getAs<FunctionProtoType>();
  const llvm::Type *Ty =
    CGM.getTypes().GetFunctionType(CGM.getTypes().getFunctionInfo(MD),
                                   FPT->isVariadic());

  llvm::Value *This = EmitLValue(E->getArg(0)).getAddress();

  llvm::Value *Callee;
  if (MD->isVirtual() && !canDevirtualizeMemberFunctionCalls(E->getArg(0)))
    Callee = BuildVirtualCall(MD, This, Ty);
  else
    Callee = CGM.GetAddrOfFunction(MD, Ty);

  return EmitCXXMemberCall(MD, Callee, This,
                           E->arg_begin() + 1, E->arg_end());
}

llvm::Value *CodeGenFunction::LoadCXXThis() {
  assert(isa<CXXMethodDecl>(CurFuncDecl) &&
         "Must be in a C++ member function decl to load 'this'");
  assert(cast<CXXMethodDecl>(CurFuncDecl)->isInstance() &&
         "Must be in a C++ member function decl to load 'this'");

  // FIXME: What if we're inside a block?
  // ans: See how CodeGenFunction::LoadObjCSelf() uses
  // CodeGenFunction::BlockForwardSelf() for how to do this.
  return Builder.CreateLoad(LocalDeclMap[CXXThisDecl], "this");
}

/// EmitCXXAggrConstructorCall - This routine essentially creates a (nested)
/// for-loop to call the default constructor on individual members of the
/// array. 
/// 'D' is the default constructor for elements of the array, 'ArrayTy' is the
/// array type and 'ArrayPtr' points to the beginning fo the array.
/// It is assumed that all relevant checks have been made by the caller.
void
CodeGenFunction::EmitCXXAggrConstructorCall(const CXXConstructorDecl *D,
                                            const ConstantArrayType *ArrayTy,
                                            llvm::Value *ArrayPtr) {
  const llvm::Type *SizeTy = ConvertType(getContext().getSizeType());
  llvm::Value * NumElements =
    llvm::ConstantInt::get(SizeTy, 
                           getContext().getConstantArrayElementCount(ArrayTy));

  EmitCXXAggrConstructorCall(D, NumElements, ArrayPtr);
}

void
CodeGenFunction::EmitCXXAggrConstructorCall(const CXXConstructorDecl *D,
                                            llvm::Value *NumElements,
                                            llvm::Value *ArrayPtr) {
  const llvm::Type *SizeTy = ConvertType(getContext().getSizeType());

  // Create a temporary for the loop index and initialize it with 0.
  llvm::Value *IndexPtr = CreateTempAlloca(SizeTy, "loop.index");
  llvm::Value *Zero = llvm::Constant::getNullValue(SizeTy);
  Builder.CreateStore(Zero, IndexPtr, false);

  // Start the loop with a block that tests the condition.
  llvm::BasicBlock *CondBlock = createBasicBlock("for.cond");
  llvm::BasicBlock *AfterFor = createBasicBlock("for.end");

  EmitBlock(CondBlock);

  llvm::BasicBlock *ForBody = createBasicBlock("for.body");

  // Generate: if (loop-index < number-of-elements fall to the loop body,
  // otherwise, go to the block after the for-loop.
  llvm::Value *Counter = Builder.CreateLoad(IndexPtr);
  llvm::Value *IsLess = Builder.CreateICmpULT(Counter, NumElements, "isless");
  // If the condition is true, execute the body.
  Builder.CreateCondBr(IsLess, ForBody, AfterFor);

  EmitBlock(ForBody);

  llvm::BasicBlock *ContinueBlock = createBasicBlock("for.inc");
  // Inside the loop body, emit the constructor call on the array element.
  Counter = Builder.CreateLoad(IndexPtr);
  llvm::Value *Address = Builder.CreateInBoundsGEP(ArrayPtr, Counter, 
                                                   "arrayidx");
  EmitCXXConstructorCall(D, Ctor_Complete, Address, 0, 0);

  EmitBlock(ContinueBlock);

  // Emit the increment of the loop counter.
  llvm::Value *NextVal = llvm::ConstantInt::get(SizeTy, 1);
  Counter = Builder.CreateLoad(IndexPtr);
  NextVal = Builder.CreateAdd(Counter, NextVal, "inc");
  Builder.CreateStore(NextVal, IndexPtr, false);

  // Finally, branch back up to the condition for the next iteration.
  EmitBranch(CondBlock);

  // Emit the fall-through block.
  EmitBlock(AfterFor, true);
}

/// EmitCXXAggrDestructorCall - calls the default destructor on array
/// elements in reverse order of construction.
void
CodeGenFunction::EmitCXXAggrDestructorCall(const CXXDestructorDecl *D,
                                           const ArrayType *Array,
                                           llvm::Value *This) {
  const ConstantArrayType *CA = dyn_cast<ConstantArrayType>(Array);
  assert(CA && "Do we support VLA for destruction ?");
  uint64_t ElementCount = getContext().getConstantArrayElementCount(CA);
  llvm::Value* ElementCountPtr =
    llvm::ConstantInt::get(llvm::Type::getInt64Ty(VMContext), ElementCount);
  EmitCXXAggrDestructorCall(D, ElementCountPtr, This);
}

/// EmitCXXAggrDestructorCall - calls the default destructor on array
/// elements in reverse order of construction.
void
CodeGenFunction::EmitCXXAggrDestructorCall(const CXXDestructorDecl *D,
                                           llvm::Value *UpperCount,
                                           llvm::Value *This) {
  llvm::Value *One = llvm::ConstantInt::get(llvm::Type::getInt64Ty(VMContext),
                                            1);
  // Create a temporary for the loop index and initialize it with count of
  // array elements.
  llvm::Value *IndexPtr = CreateTempAlloca(llvm::Type::getInt64Ty(VMContext),
                                           "loop.index");
  // Index = ElementCount;
  Builder.CreateStore(UpperCount, IndexPtr, false);

  // Start the loop with a block that tests the condition.
  llvm::BasicBlock *CondBlock = createBasicBlock("for.cond");
  llvm::BasicBlock *AfterFor = createBasicBlock("for.end");

  EmitBlock(CondBlock);

  llvm::BasicBlock *ForBody = createBasicBlock("for.body");

  // Generate: if (loop-index != 0 fall to the loop body,
  // otherwise, go to the block after the for-loop.
  llvm::Value* zeroConstant =
    llvm::Constant::getNullValue(llvm::Type::getInt64Ty(VMContext));
  llvm::Value *Counter = Builder.CreateLoad(IndexPtr);
  llvm::Value *IsNE = Builder.CreateICmpNE(Counter, zeroConstant,
                                            "isne");
  // If the condition is true, execute the body.
  Builder.CreateCondBr(IsNE, ForBody, AfterFor);

  EmitBlock(ForBody);

  llvm::BasicBlock *ContinueBlock = createBasicBlock("for.inc");
  // Inside the loop body, emit the constructor call on the array element.
  Counter = Builder.CreateLoad(IndexPtr);
  Counter = Builder.CreateSub(Counter, One);
  llvm::Value *Address = Builder.CreateInBoundsGEP(This, Counter, "arrayidx");
  if (D->isVirtual()) {
    const llvm::Type *Ty =
      CGM.getTypes().GetFunctionType(CGM.getTypes().getFunctionInfo(D),
                                     /*isVariadic=*/false);
    
    llvm::Value *Callee = BuildVirtualCall(D, Dtor_Deleting, Address, Ty);
    EmitCXXMemberCall(D, Callee, Address, 0, 0);
  }
  else
    EmitCXXDestructorCall(D, Dtor_Complete, Address);

  EmitBlock(ContinueBlock);

  // Emit the decrement of the loop counter.
  Counter = Builder.CreateLoad(IndexPtr);
  Counter = Builder.CreateSub(Counter, One, "dec");
  Builder.CreateStore(Counter, IndexPtr, false);

  // Finally, branch back up to the condition for the next iteration.
  EmitBranch(CondBlock);

  // Emit the fall-through block.
  EmitBlock(AfterFor, true);
}

/// GenerateCXXAggrDestructorHelper - Generates a helper function which when
/// invoked, calls the default destructor on array elements in reverse order of
/// construction.
llvm::Constant * 
CodeGenFunction::GenerateCXXAggrDestructorHelper(const CXXDestructorDecl *D,
                                                 const ArrayType *Array,
                                                 llvm::Value *This) {
  static int UniqueCount;
  FunctionArgList Args;
  ImplicitParamDecl *Dst =
    ImplicitParamDecl::Create(getContext(), 0,
                              SourceLocation(), 0,
                              getContext().getPointerType(getContext().VoidTy));
  Args.push_back(std::make_pair(Dst, Dst->getType()));
  
  llvm::SmallString<16> Name;
  llvm::raw_svector_ostream(Name) << "__tcf_" << (++UniqueCount);
  QualType R = getContext().VoidTy;
  const CGFunctionInfo &FI = CGM.getTypes().getFunctionInfo(R, Args);
  const llvm::FunctionType *FTy = CGM.getTypes().GetFunctionType(FI, false);
  llvm::Function *Fn =
    llvm::Function::Create(FTy, llvm::GlobalValue::InternalLinkage,
                           Name.c_str(),
                           &CGM.getModule());
  IdentifierInfo *II
    = &CGM.getContext().Idents.get(Name.c_str());
  FunctionDecl *FD = FunctionDecl::Create(getContext(),
                                          getContext().getTranslationUnitDecl(),
                                          SourceLocation(), II, R, 0,
                                          FunctionDecl::Static,
                                          false, true);
  StartFunction(FD, R, Fn, Args, SourceLocation());
  QualType BaseElementTy = getContext().getBaseElementType(Array);
  const llvm::Type *BasePtr = ConvertType(BaseElementTy);
  BasePtr = llvm::PointerType::getUnqual(BasePtr);
  llvm::Value *BaseAddrPtr = Builder.CreateBitCast(This, BasePtr);
  EmitCXXAggrDestructorCall(D, Array, BaseAddrPtr);
  FinishFunction();
  llvm::Type *Ptr8Ty = llvm::PointerType::get(llvm::Type::getInt8Ty(VMContext),
                                              0);
  llvm::Constant *m = llvm::ConstantExpr::getBitCast(Fn, Ptr8Ty);
  return m;
}

void
CodeGenFunction::EmitCXXConstructorCall(const CXXConstructorDecl *D,
                                        CXXCtorType Type,
                                        llvm::Value *This,
                                        CallExpr::const_arg_iterator ArgBeg,
                                        CallExpr::const_arg_iterator ArgEnd) {
  if (D->isCopyConstructor(getContext())) {
    const CXXRecordDecl *ClassDecl = cast<CXXRecordDecl>(D->getDeclContext());
    if (ClassDecl->hasTrivialCopyConstructor()) {
      assert(!ClassDecl->hasUserDeclaredCopyConstructor() &&
             "EmitCXXConstructorCall - user declared copy constructor");
      const Expr *E = (*ArgBeg);
      QualType Ty = E->getType();
      llvm::Value *Src = EmitLValue(E).getAddress();
      EmitAggregateCopy(This, Src, Ty);
      return;
    }
  }

  llvm::Value *Callee = CGM.GetAddrOfCXXConstructor(D, Type);

  EmitCXXMemberCall(D, Callee, This, ArgBeg, ArgEnd);
}

void CodeGenFunction::EmitCXXDestructorCall(const CXXDestructorDecl *D,
                                            CXXDtorType Type,
                                            llvm::Value *This) {
  llvm::Value *Callee = CGM.GetAddrOfCXXDestructor(D, Type);

  EmitCXXMemberCall(D, Callee, This, 0, 0);
}

void
CodeGenFunction::EmitCXXConstructExpr(llvm::Value *Dest,
                                      const CXXConstructExpr *E) {
  assert(Dest && "Must have a destination!");
  const CXXConstructorDecl *CD = E->getConstructor();
  const ConstantArrayType *Array =
    getContext().getAsConstantArrayType(E->getType());
  // For a copy constructor, even if it is trivial, must fall thru so
  // its argument is code-gen'ed.
  if (!CD->isCopyConstructor(getContext())) {
    QualType InitType = E->getType();
    if (Array)
      InitType = getContext().getBaseElementType(Array);
    const CXXRecordDecl *RD =
      cast<CXXRecordDecl>(InitType->getAs<RecordType>()->getDecl());
    if (RD->hasTrivialConstructor())
    return;
  }
  // Code gen optimization to eliminate copy constructor and return
  // its first argument instead.
  if (getContext().getLangOptions().ElideConstructors && E->isElidable()) {
    const Expr *Arg = E->getArg(0);
    
    if (const CXXBindTemporaryExpr *BindExpr = 
          dyn_cast<CXXBindTemporaryExpr>(Arg))
      Arg = BindExpr->getSubExpr();

    EmitAggExpr(Arg, Dest, false);
    return;
  }
  if (Array) {
    QualType BaseElementTy = getContext().getBaseElementType(Array);
    const llvm::Type *BasePtr = ConvertType(BaseElementTy);
    BasePtr = llvm::PointerType::getUnqual(BasePtr);
    llvm::Value *BaseAddrPtr =
      Builder.CreateBitCast(Dest, BasePtr);
    EmitCXXAggrConstructorCall(CD, Array, BaseAddrPtr);
  }
  else
    // Call the constructor.
    EmitCXXConstructorCall(CD, Ctor_Complete, Dest,
                           E->arg_begin(), E->arg_end());
}

void CodeGenModule::EmitCXXConstructors(const CXXConstructorDecl *D) {
  EmitGlobal(GlobalDecl(D, Ctor_Complete));
  EmitGlobal(GlobalDecl(D, Ctor_Base));
}

void CodeGenModule::EmitCXXConstructor(const CXXConstructorDecl *D,
                                       CXXCtorType Type) {

  llvm::Function *Fn = GetAddrOfCXXConstructor(D, Type);

  CodeGenFunction(*this).GenerateCode(GlobalDecl(D, Type), Fn);

  SetFunctionDefinitionAttributes(D, Fn);
  SetLLVMFunctionAttributesForDefinition(D, Fn);
}

llvm::Function *
CodeGenModule::GetAddrOfCXXConstructor(const CXXConstructorDecl *D,
                                       CXXCtorType Type) {
  const FunctionProtoType *FPT = D->getType()->getAs<FunctionProtoType>();
  const llvm::FunctionType *FTy =
    getTypes().GetFunctionType(getTypes().getFunctionInfo(D), 
                               FPT->isVariadic());

  const char *Name = getMangledCXXCtorName(D, Type);
  return cast<llvm::Function>(
                      GetOrCreateLLVMFunction(Name, FTy, GlobalDecl(D, Type)));
}

const char *CodeGenModule::getMangledCXXCtorName(const CXXConstructorDecl *D,
                                                 CXXCtorType Type) {
  llvm::SmallString<256> Name;
  getMangleContext().mangleCXXCtor(D, Type, Name);

  Name += '\0';
  return UniqueMangledName(Name.begin(), Name.end());
}

void CodeGenModule::EmitCXXDestructors(const CXXDestructorDecl *D) {
  if (D->isVirtual())
    EmitCXXDestructor(D, Dtor_Deleting);
  EmitCXXDestructor(D, Dtor_Complete);
  EmitCXXDestructor(D, Dtor_Base);
}

void CodeGenModule::EmitCXXDestructor(const CXXDestructorDecl *D,
                                      CXXDtorType Type) {
  llvm::Function *Fn = GetAddrOfCXXDestructor(D, Type);

  CodeGenFunction(*this).GenerateCode(GlobalDecl(D, Type), Fn);

  SetFunctionDefinitionAttributes(D, Fn);
  SetLLVMFunctionAttributesForDefinition(D, Fn);
}

llvm::Function *
CodeGenModule::GetAddrOfCXXDestructor(const CXXDestructorDecl *D,
                                      CXXDtorType Type) {
  const llvm::FunctionType *FTy =
    getTypes().GetFunctionType(getTypes().getFunctionInfo(D), false);

  const char *Name = getMangledCXXDtorName(D, Type);
  return cast<llvm::Function>(
                      GetOrCreateLLVMFunction(Name, FTy, GlobalDecl(D, Type)));
}

const char *CodeGenModule::getMangledCXXDtorName(const CXXDestructorDecl *D,
                                                 CXXDtorType Type) {
  llvm::SmallString<256> Name;
  getMangleContext().mangleCXXDtor(D, Type, Name);

  Name += '\0';
  return UniqueMangledName(Name.begin(), Name.end());
}

llvm::Constant *CodeGenFunction::GenerateThunk(llvm::Function *Fn,
                                               const CXXMethodDecl *MD,
                                               bool Extern, int64_t nv,
                                               int64_t v) {
  return GenerateCovariantThunk(Fn, MD, Extern, nv, v, 0, 0);
}

llvm::Value *CodeGenFunction::DynamicTypeAdjust(llvm::Value *V, int64_t nv,
                                                int64_t v) {
  llvm::Type *Ptr8Ty = llvm::PointerType::get(llvm::Type::getInt8Ty(VMContext),
                                              0);
  const llvm::Type *OrigTy = V->getType();
  if (nv) {
    // Do the non-virtual adjustment
    V = Builder.CreateBitCast(V, Ptr8Ty);
    V = Builder.CreateConstInBoundsGEP1_64(V, nv);
    V = Builder.CreateBitCast(V, OrigTy);
  }
  if (v) {
    // Do the virtual this adjustment
    const llvm::Type *PtrDiffTy = 
      ConvertType(getContext().getPointerDiffType());
    llvm::Type *PtrPtr8Ty, *PtrPtrDiffTy;
    PtrPtr8Ty = llvm::PointerType::get(Ptr8Ty, 0);
    PtrPtrDiffTy = llvm::PointerType::get(PtrDiffTy, 0);
    llvm::Value *ThisVal = Builder.CreateBitCast(V, Ptr8Ty);
    V = Builder.CreateBitCast(V, PtrPtrDiffTy->getPointerTo());
    V = Builder.CreateLoad(V, "vtable");
    llvm::Value *VTablePtr = V;
    assert(v % (LLVMPointerWidth/8) == 0 && "vtable entry unaligned");
    v /= LLVMPointerWidth/8;
    V = Builder.CreateConstInBoundsGEP1_64(VTablePtr, v);
    V = Builder.CreateLoad(V);
    V = Builder.CreateGEP(ThisVal, V);
    V = Builder.CreateBitCast(V, OrigTy);
  }
  return V;
}

llvm::Constant *CodeGenFunction::GenerateCovariantThunk(llvm::Function *Fn,
                                                        const CXXMethodDecl *MD,
                                                        bool Extern,
                                                        int64_t nv_t,
                                                        int64_t v_t,
                                                        int64_t nv_r,
                                                        int64_t v_r) {
  QualType ResultType = MD->getType()->getAs<FunctionType>()->getResultType();

  FunctionArgList Args;
  ImplicitParamDecl *ThisDecl =
    ImplicitParamDecl::Create(getContext(), 0, SourceLocation(), 0,
                              MD->getThisType(getContext()));
  Args.push_back(std::make_pair(ThisDecl, ThisDecl->getType()));
  for (FunctionDecl::param_const_iterator i = MD->param_begin(),
         e = MD->param_end();
       i != e; ++i) {
    ParmVarDecl *D = *i;
    Args.push_back(std::make_pair(D, D->getType()));
  }
  IdentifierInfo *II
    = &CGM.getContext().Idents.get("__thunk_named_foo_");
  FunctionDecl *FD = FunctionDecl::Create(getContext(),
                                          getContext().getTranslationUnitDecl(),
                                          SourceLocation(), II, ResultType, 0,
                                          Extern
                                            ? FunctionDecl::Extern
                                            : FunctionDecl::Static,
                                          false, true);
  StartFunction(FD, ResultType, Fn, Args, SourceLocation());

  // generate body
  const FunctionProtoType *FPT = MD->getType()->getAs<FunctionProtoType>();
  const llvm::Type *Ty =
    CGM.getTypes().GetFunctionType(CGM.getTypes().getFunctionInfo(MD),
                                   FPT->isVariadic());
  llvm::Value *Callee = CGM.GetAddrOfFunction(MD, Ty);
  CallArgList CallArgs;

  QualType ArgType = MD->getThisType(getContext());
  llvm::Value *Arg = Builder.CreateLoad(LocalDeclMap[ThisDecl], "this");
  if (nv_t || v_t) {
    // Do the this adjustment.
    const llvm::Type *OrigTy = Callee->getType();
    Arg = DynamicTypeAdjust(Arg, nv_t, v_t);
    if (nv_r || v_r) {
      Callee = CGM.BuildCovariantThunk(MD, Extern, 0, 0, nv_r, v_r);
      Callee = Builder.CreateBitCast(Callee, OrigTy);
      nv_r = v_r = 0;
    }
  }    

  CallArgs.push_back(std::make_pair(RValue::get(Arg), ArgType));

  for (FunctionDecl::param_const_iterator i = MD->param_begin(),
         e = MD->param_end();
       i != e; ++i) {
    ParmVarDecl *D = *i;
    QualType ArgType = D->getType();

    // llvm::Value *Arg = CGF.GetAddrOfLocalVar(Dst);
    Expr *Arg = new (getContext()) DeclRefExpr(D, ArgType, SourceLocation());
    CallArgs.push_back(std::make_pair(EmitCallArg(Arg, ArgType), ArgType));
  }

  RValue RV = EmitCall(CGM.getTypes().getFunctionInfo(ResultType, CallArgs),
                       Callee, CallArgs, MD);
  if (nv_r || v_r) {
    bool CanBeZero = !(ResultType->isReferenceType()
    // FIXME: attr nonnull can't be zero either
                       /* || ResultType->hasAttr<NonNullAttr>() */ );
    // Do the return result adjustment.
    if (CanBeZero) {
      llvm::BasicBlock *NonZeroBlock = createBasicBlock();
      llvm::BasicBlock *ZeroBlock = createBasicBlock();
      llvm::BasicBlock *ContBlock = createBasicBlock();

      const llvm::Type *Ty = RV.getScalarVal()->getType();
      llvm::Value *Zero = llvm::Constant::getNullValue(Ty);
      Builder.CreateCondBr(Builder.CreateICmpNE(RV.getScalarVal(), Zero),
                           NonZeroBlock, ZeroBlock);
      EmitBlock(NonZeroBlock);
      llvm::Value *NZ = DynamicTypeAdjust(RV.getScalarVal(), nv_r, v_r);
      EmitBranch(ContBlock);
      EmitBlock(ZeroBlock);
      llvm::Value *Z = RV.getScalarVal();
      EmitBlock(ContBlock);
      llvm::PHINode *RVOrZero = Builder.CreatePHI(Ty);
      RVOrZero->reserveOperandSpace(2);
      RVOrZero->addIncoming(NZ, NonZeroBlock);
      RVOrZero->addIncoming(Z, ZeroBlock);
      RV = RValue::get(RVOrZero);
    } else
      RV = RValue::get(DynamicTypeAdjust(RV.getScalarVal(), nv_r, v_r));
  }

  if (!ResultType->isVoidType())
    EmitReturnOfRValue(RV, ResultType);

  FinishFunction();
  return Fn;
}

llvm::Constant *CodeGenModule::BuildThunk(const CXXMethodDecl *MD, bool Extern,
                                          int64_t nv, int64_t v) {
  llvm::SmallString<256> OutName;
  getMangleContext().mangleThunk(MD, nv, v, OutName);
  llvm::GlobalVariable::LinkageTypes linktype;
  linktype = llvm::GlobalValue::WeakAnyLinkage;
  if (!Extern)
    linktype = llvm::GlobalValue::InternalLinkage;
  llvm::Type *Ptr8Ty=llvm::PointerType::get(llvm::Type::getInt8Ty(VMContext),0);
  const FunctionProtoType *FPT = MD->getType()->getAs<FunctionProtoType>();
  const llvm::FunctionType *FTy =
    getTypes().GetFunctionType(getTypes().getFunctionInfo(MD),
                               FPT->isVariadic());

  llvm::Function *Fn = llvm::Function::Create(FTy, linktype, OutName.str(),
                                              &getModule());
  CodeGenFunction(*this).GenerateThunk(Fn, MD, Extern, nv, v);
  llvm::Constant *m = llvm::ConstantExpr::getBitCast(Fn, Ptr8Ty);
  return m;
}

llvm::Constant *CodeGenModule::BuildCovariantThunk(const CXXMethodDecl *MD,
                                                   bool Extern, int64_t nv_t,
                                                   int64_t v_t, int64_t nv_r,
                                                   int64_t v_r) {
  llvm::SmallString<256> OutName;
  getMangleContext().mangleCovariantThunk(MD, nv_t, v_t, nv_r, v_r, OutName);
  llvm::GlobalVariable::LinkageTypes linktype;
  linktype = llvm::GlobalValue::WeakAnyLinkage;
  if (!Extern)
    linktype = llvm::GlobalValue::InternalLinkage;
  llvm::Type *Ptr8Ty=llvm::PointerType::get(llvm::Type::getInt8Ty(VMContext),0);
  const FunctionProtoType *FPT = MD->getType()->getAs<FunctionProtoType>();
  const llvm::FunctionType *FTy =
    getTypes().GetFunctionType(getTypes().getFunctionInfo(MD),
                               FPT->isVariadic());

  llvm::Function *Fn = llvm::Function::Create(FTy, linktype, OutName.str(),
                                              &getModule());
  CodeGenFunction(*this).GenerateCovariantThunk(Fn, MD, Extern, nv_t, v_t, nv_r,
                                               v_r);
  llvm::Constant *m = llvm::ConstantExpr::getBitCast(Fn, Ptr8Ty);
  return m;
}

llvm::Value *
CodeGenFunction::GetVirtualCXXBaseClassOffset(llvm::Value *This,
                                              const CXXRecordDecl *ClassDecl,
                                           const CXXRecordDecl *BaseClassDecl) {
  const llvm::Type *Int8PtrTy = 
    llvm::Type::getInt8Ty(VMContext)->getPointerTo();

  llvm::Value *VTablePtr = Builder.CreateBitCast(This, 
                                                 Int8PtrTy->getPointerTo());
  VTablePtr = Builder.CreateLoad(VTablePtr, "vtable");

  int64_t VBaseOffsetIndex = 
    CGM.getVtableInfo().getVirtualBaseOffsetIndex(ClassDecl, BaseClassDecl);
  
  llvm::Value *VBaseOffsetPtr = 
    Builder.CreateConstGEP1_64(VTablePtr, VBaseOffsetIndex, "vbase.offset.ptr");
  const llvm::Type *PtrDiffTy = 
    ConvertType(getContext().getPointerDiffType());
  
  VBaseOffsetPtr = Builder.CreateBitCast(VBaseOffsetPtr, 
                                         PtrDiffTy->getPointerTo());
                                         
  llvm::Value *VBaseOffset = Builder.CreateLoad(VBaseOffsetPtr, "vbase.offset");
  
  return VBaseOffset;
}

static llvm::Value *BuildVirtualCall(CodeGenFunction &CGF, int64_t VtableIndex, 
                                     llvm::Value *This, const llvm::Type *Ty) {
  Ty = Ty->getPointerTo()->getPointerTo()->getPointerTo();
  
  llvm::Value *Vtable = CGF.Builder.CreateBitCast(This, Ty);
  Vtable = CGF.Builder.CreateLoad(Vtable);
  
  llvm::Value *VFuncPtr = 
    CGF.Builder.CreateConstInBoundsGEP1_64(Vtable, VtableIndex, "vfn");
  return CGF.Builder.CreateLoad(VFuncPtr);
}

llvm::Value *
CodeGenFunction::BuildVirtualCall(const CXXMethodDecl *MD, llvm::Value *This,
                                  const llvm::Type *Ty) {
  MD = MD->getCanonicalDecl();
  int64_t VtableIndex = CGM.getVtableInfo().getMethodVtableIndex(MD);
  
  return ::BuildVirtualCall(*this, VtableIndex, This, Ty);
}

llvm::Value *
CodeGenFunction::BuildVirtualCall(const CXXDestructorDecl *DD, CXXDtorType Type, 
                                  llvm::Value *&This, const llvm::Type *Ty) {
  DD = cast<CXXDestructorDecl>(DD->getCanonicalDecl());
  int64_t VtableIndex = 
    CGM.getVtableInfo().getMethodVtableIndex(GlobalDecl(DD, Type));

  return ::BuildVirtualCall(*this, VtableIndex, This, Ty);
}

/// EmitClassAggrMemberwiseCopy - This routine generates code to copy a class
/// array of objects from SrcValue to DestValue. Copying can be either a bitwise
/// copy or via a copy constructor call.
//  FIXME. Consolidate this with EmitCXXAggrConstructorCall.
void CodeGenFunction::EmitClassAggrMemberwiseCopy(llvm::Value *Dest,
                                            llvm::Value *Src,
                                            const ArrayType *Array,
                                            const CXXRecordDecl *BaseClassDecl,
                                            QualType Ty) {
  const ConstantArrayType *CA = dyn_cast<ConstantArrayType>(Array);
  assert(CA && "VLA cannot be copied over");
  bool BitwiseCopy = BaseClassDecl->hasTrivialCopyConstructor();

  // Create a temporary for the loop index and initialize it with 0.
  llvm::Value *IndexPtr = CreateTempAlloca(llvm::Type::getInt64Ty(VMContext),
                                           "loop.index");
  llvm::Value* zeroConstant =
    llvm::Constant::getNullValue(llvm::Type::getInt64Ty(VMContext));
  Builder.CreateStore(zeroConstant, IndexPtr, false);
  // Start the loop with a block that tests the condition.
  llvm::BasicBlock *CondBlock = createBasicBlock("for.cond");
  llvm::BasicBlock *AfterFor = createBasicBlock("for.end");

  EmitBlock(CondBlock);

  llvm::BasicBlock *ForBody = createBasicBlock("for.body");
  // Generate: if (loop-index < number-of-elements fall to the loop body,
  // otherwise, go to the block after the for-loop.
  uint64_t NumElements = getContext().getConstantArrayElementCount(CA);
  llvm::Value * NumElementsPtr =
    llvm::ConstantInt::get(llvm::Type::getInt64Ty(VMContext), NumElements);
  llvm::Value *Counter = Builder.CreateLoad(IndexPtr);
  llvm::Value *IsLess = Builder.CreateICmpULT(Counter, NumElementsPtr,
                                              "isless");
  // If the condition is true, execute the body.
  Builder.CreateCondBr(IsLess, ForBody, AfterFor);

  EmitBlock(ForBody);
  llvm::BasicBlock *ContinueBlock = createBasicBlock("for.inc");
  // Inside the loop body, emit the constructor call on the array element.
  Counter = Builder.CreateLoad(IndexPtr);
  Src = Builder.CreateInBoundsGEP(Src, Counter, "srcaddress");
  Dest = Builder.CreateInBoundsGEP(Dest, Counter, "destaddress");
  if (BitwiseCopy)
    EmitAggregateCopy(Dest, Src, Ty);
  else if (CXXConstructorDecl *BaseCopyCtor =
           BaseClassDecl->getCopyConstructor(getContext(), 0)) {
    llvm::Value *Callee = CGM.GetAddrOfCXXConstructor(BaseCopyCtor,
                                                      Ctor_Complete);
    CallArgList CallArgs;
    // Push the this (Dest) ptr.
    CallArgs.push_back(std::make_pair(RValue::get(Dest),
                                      BaseCopyCtor->getThisType(getContext())));

    // Push the Src ptr.
    CallArgs.push_back(std::make_pair(RValue::get(Src),
                                     BaseCopyCtor->getParamDecl(0)->getType()));
    QualType ResultType =
      BaseCopyCtor->getType()->getAs<FunctionType>()->getResultType();
    EmitCall(CGM.getTypes().getFunctionInfo(ResultType, CallArgs),
             Callee, CallArgs, BaseCopyCtor);
  }
  EmitBlock(ContinueBlock);

  // Emit the increment of the loop counter.
  llvm::Value *NextVal = llvm::ConstantInt::get(Counter->getType(), 1);
  Counter = Builder.CreateLoad(IndexPtr);
  NextVal = Builder.CreateAdd(Counter, NextVal, "inc");
  Builder.CreateStore(NextVal, IndexPtr, false);

  // Finally, branch back up to the condition for the next iteration.
  EmitBranch(CondBlock);

  // Emit the fall-through block.
  EmitBlock(AfterFor, true);
}

/// EmitClassAggrCopyAssignment - This routine generates code to assign a class
/// array of objects from SrcValue to DestValue. Assignment can be either a
/// bitwise assignment or via a copy assignment operator function call.
/// FIXME. This can be consolidated with EmitClassAggrMemberwiseCopy
void CodeGenFunction::EmitClassAggrCopyAssignment(llvm::Value *Dest,
                                            llvm::Value *Src,
                                            const ArrayType *Array,
                                            const CXXRecordDecl *BaseClassDecl,
                                            QualType Ty) {
  const ConstantArrayType *CA = dyn_cast<ConstantArrayType>(Array);
  assert(CA && "VLA cannot be asssigned");
  bool BitwiseAssign = BaseClassDecl->hasTrivialCopyAssignment();

  // Create a temporary for the loop index and initialize it with 0.
  llvm::Value *IndexPtr = CreateTempAlloca(llvm::Type::getInt64Ty(VMContext),
                                           "loop.index");
  llvm::Value* zeroConstant =
  llvm::Constant::getNullValue(llvm::Type::getInt64Ty(VMContext));
  Builder.CreateStore(zeroConstant, IndexPtr, false);
  // Start the loop with a block that tests the condition.
  llvm::BasicBlock *CondBlock = createBasicBlock("for.cond");
  llvm::BasicBlock *AfterFor = createBasicBlock("for.end");

  EmitBlock(CondBlock);

  llvm::BasicBlock *ForBody = createBasicBlock("for.body");
  // Generate: if (loop-index < number-of-elements fall to the loop body,
  // otherwise, go to the block after the for-loop.
  uint64_t NumElements = getContext().getConstantArrayElementCount(CA);
  llvm::Value * NumElementsPtr =
  llvm::ConstantInt::get(llvm::Type::getInt64Ty(VMContext), NumElements);
  llvm::Value *Counter = Builder.CreateLoad(IndexPtr);
  llvm::Value *IsLess = Builder.CreateICmpULT(Counter, NumElementsPtr,
                                              "isless");
  // If the condition is true, execute the body.
  Builder.CreateCondBr(IsLess, ForBody, AfterFor);

  EmitBlock(ForBody);
  llvm::BasicBlock *ContinueBlock = createBasicBlock("for.inc");
  // Inside the loop body, emit the assignment operator call on array element.
  Counter = Builder.CreateLoad(IndexPtr);
  Src = Builder.CreateInBoundsGEP(Src, Counter, "srcaddress");
  Dest = Builder.CreateInBoundsGEP(Dest, Counter, "destaddress");
  const CXXMethodDecl *MD = 0;
  if (BitwiseAssign)
    EmitAggregateCopy(Dest, Src, Ty);
  else {
    bool hasCopyAssign = BaseClassDecl->hasConstCopyAssignment(getContext(),
                                                               MD);
    assert(hasCopyAssign && "EmitClassAggrCopyAssignment - No user assign");
    (void)hasCopyAssign;
    const FunctionProtoType *FPT = MD->getType()->getAs<FunctionProtoType>();
    const llvm::Type *LTy =
    CGM.getTypes().GetFunctionType(CGM.getTypes().getFunctionInfo(MD),
                                   FPT->isVariadic());
    llvm::Constant *Callee = CGM.GetAddrOfFunction(MD, LTy);

    CallArgList CallArgs;
    // Push the this (Dest) ptr.
    CallArgs.push_back(std::make_pair(RValue::get(Dest),
                                      MD->getThisType(getContext())));

    // Push the Src ptr.
    CallArgs.push_back(std::make_pair(RValue::get(Src),
                                      MD->getParamDecl(0)->getType()));
    QualType ResultType = MD->getType()->getAs<FunctionType>()->getResultType();
    EmitCall(CGM.getTypes().getFunctionInfo(ResultType, CallArgs),
             Callee, CallArgs, MD);
  }
  EmitBlock(ContinueBlock);

  // Emit the increment of the loop counter.
  llvm::Value *NextVal = llvm::ConstantInt::get(Counter->getType(), 1);
  Counter = Builder.CreateLoad(IndexPtr);
  NextVal = Builder.CreateAdd(Counter, NextVal, "inc");
  Builder.CreateStore(NextVal, IndexPtr, false);

  // Finally, branch back up to the condition for the next iteration.
  EmitBranch(CondBlock);

  // Emit the fall-through block.
  EmitBlock(AfterFor, true);
}

/// EmitClassMemberwiseCopy - This routine generates code to copy a class
/// object from SrcValue to DestValue. Copying can be either a bitwise copy
/// or via a copy constructor call.
void CodeGenFunction::EmitClassMemberwiseCopy(
                        llvm::Value *Dest, llvm::Value *Src,
                        const CXXRecordDecl *ClassDecl,
                        const CXXRecordDecl *BaseClassDecl, QualType Ty) {
  if (ClassDecl) {
    Dest = GetAddressOfBaseClass(Dest, ClassDecl, BaseClassDecl,
                                 /*NullCheckValue=*/false);
    Src = GetAddressOfBaseClass(Src, ClassDecl, BaseClassDecl,
                                /*NullCheckValue=*/false);
  }
  if (BaseClassDecl->hasTrivialCopyConstructor()) {
    EmitAggregateCopy(Dest, Src, Ty);
    return;
  }

  if (CXXConstructorDecl *BaseCopyCtor =
      BaseClassDecl->getCopyConstructor(getContext(), 0)) {
    llvm::Value *Callee = CGM.GetAddrOfCXXConstructor(BaseCopyCtor,
                                                      Ctor_Complete);
    CallArgList CallArgs;
    // Push the this (Dest) ptr.
    CallArgs.push_back(std::make_pair(RValue::get(Dest),
                                      BaseCopyCtor->getThisType(getContext())));

    // Push the Src ptr.
    CallArgs.push_back(std::make_pair(RValue::get(Src),
                       BaseCopyCtor->getParamDecl(0)->getType()));
    QualType ResultType =
    BaseCopyCtor->getType()->getAs<FunctionType>()->getResultType();
    EmitCall(CGM.getTypes().getFunctionInfo(ResultType, CallArgs),
             Callee, CallArgs, BaseCopyCtor);
  }
}

/// EmitClassCopyAssignment - This routine generates code to copy assign a class
/// object from SrcValue to DestValue. Assignment can be either a bitwise
/// assignment of via an assignment operator call.
// FIXME. Consolidate this with EmitClassMemberwiseCopy as they share a lot.
void CodeGenFunction::EmitClassCopyAssignment(
                                        llvm::Value *Dest, llvm::Value *Src,
                                        const CXXRecordDecl *ClassDecl,
                                        const CXXRecordDecl *BaseClassDecl,
                                        QualType Ty) {
  if (ClassDecl) {
    Dest = GetAddressOfBaseClass(Dest, ClassDecl, BaseClassDecl,
                                 /*NullCheckValue=*/false);
    Src = GetAddressOfBaseClass(Src, ClassDecl, BaseClassDecl,
                                /*NullCheckValue=*/false);
  }
  if (BaseClassDecl->hasTrivialCopyAssignment()) {
    EmitAggregateCopy(Dest, Src, Ty);
    return;
  }

  const CXXMethodDecl *MD = 0;
  bool ConstCopyAssignOp = BaseClassDecl->hasConstCopyAssignment(getContext(),
                                                                 MD);
  assert(ConstCopyAssignOp && "EmitClassCopyAssignment - missing copy assign");
  (void)ConstCopyAssignOp;

  const FunctionProtoType *FPT = MD->getType()->getAs<FunctionProtoType>();
  const llvm::Type *LTy =
    CGM.getTypes().GetFunctionType(CGM.getTypes().getFunctionInfo(MD),
                                   FPT->isVariadic());
  llvm::Constant *Callee = CGM.GetAddrOfFunction(MD, LTy);

  CallArgList CallArgs;
  // Push the this (Dest) ptr.
  CallArgs.push_back(std::make_pair(RValue::get(Dest),
                                    MD->getThisType(getContext())));

  // Push the Src ptr.
  CallArgs.push_back(std::make_pair(RValue::get(Src),
                                    MD->getParamDecl(0)->getType()));
  QualType ResultType =
    MD->getType()->getAs<FunctionType>()->getResultType();
  EmitCall(CGM.getTypes().getFunctionInfo(ResultType, CallArgs),
           Callee, CallArgs, MD);
}

/// SynthesizeDefaultConstructor - synthesize a default constructor
void
CodeGenFunction::SynthesizeDefaultConstructor(const CXXConstructorDecl *Ctor,
                                              CXXCtorType Type,
                                              llvm::Function *Fn,
                                              const FunctionArgList &Args) {
  StartFunction(GlobalDecl(Ctor, Type), Ctor->getResultType(), Fn, Args, 
                SourceLocation());
  EmitCtorPrologue(Ctor, Type);
  FinishFunction();
}

/// SynthesizeCXXCopyConstructor - This routine implicitly defines body of a
/// copy constructor, in accordance with section 12.8 (p7 and p8) of C++03
/// The implicitly-defined copy constructor for class X performs a memberwise
/// copy of its subobjects. The order of copying is the same as the order of
/// initialization of bases and members in a user-defined constructor
/// Each subobject is copied in the manner appropriate to its type:
///  if the subobject is of class type, the copy constructor for the class is
///  used;
///  if the subobject is an array, each element is copied, in the manner
///  appropriate to the element type;
///  if the subobject is of scalar type, the built-in assignment operator is
///  used.
/// Virtual base class subobjects shall be copied only once by the
/// implicitly-defined copy constructor

void 
CodeGenFunction::SynthesizeCXXCopyConstructor(const CXXConstructorDecl *Ctor,
                                              CXXCtorType Type,
                                              llvm::Function *Fn,
                                              const FunctionArgList &Args) {
  const CXXRecordDecl *ClassDecl = Ctor->getParent();
  assert(!ClassDecl->hasUserDeclaredCopyConstructor() &&
      "SynthesizeCXXCopyConstructor - copy constructor has definition already");
  StartFunction(GlobalDecl(Ctor, Type), Ctor->getResultType(), Fn, Args, 
                SourceLocation());

  FunctionArgList::const_iterator i = Args.begin();
  const VarDecl *ThisArg = i->first;
  llvm::Value *ThisObj = GetAddrOfLocalVar(ThisArg);
  llvm::Value *LoadOfThis = Builder.CreateLoad(ThisObj, "this");
  const VarDecl *SrcArg = (i+1)->first;
  llvm::Value *SrcObj = GetAddrOfLocalVar(SrcArg);
  llvm::Value *LoadOfSrc = Builder.CreateLoad(SrcObj);

  for (CXXRecordDecl::base_class_const_iterator Base = ClassDecl->bases_begin();
       Base != ClassDecl->bases_end(); ++Base) {
    // FIXME. copy constrution of virtual base NYI
    if (Base->isVirtual())
      continue;

    CXXRecordDecl *BaseClassDecl
      = cast<CXXRecordDecl>(Base->getType()->getAs<RecordType>()->getDecl());
    EmitClassMemberwiseCopy(LoadOfThis, LoadOfSrc, ClassDecl, BaseClassDecl,
                            Base->getType());
  }

  for (CXXRecordDecl::field_iterator Field = ClassDecl->field_begin(),
       FieldEnd = ClassDecl->field_end();
       Field != FieldEnd; ++Field) {
    QualType FieldType = getContext().getCanonicalType((*Field)->getType());
    const ConstantArrayType *Array =
      getContext().getAsConstantArrayType(FieldType);
    if (Array)
      FieldType = getContext().getBaseElementType(FieldType);

    if (const RecordType *FieldClassType = FieldType->getAs<RecordType>()) {
      CXXRecordDecl *FieldClassDecl
        = cast<CXXRecordDecl>(FieldClassType->getDecl());
      LValue LHS = EmitLValueForField(LoadOfThis, *Field, false, 0);
      LValue RHS = EmitLValueForField(LoadOfSrc, *Field, false, 0);
      if (Array) {
        const llvm::Type *BasePtr = ConvertType(FieldType);
        BasePtr = llvm::PointerType::getUnqual(BasePtr);
        llvm::Value *DestBaseAddrPtr =
          Builder.CreateBitCast(LHS.getAddress(), BasePtr);
        llvm::Value *SrcBaseAddrPtr =
          Builder.CreateBitCast(RHS.getAddress(), BasePtr);
        EmitClassAggrMemberwiseCopy(DestBaseAddrPtr, SrcBaseAddrPtr, Array,
                                    FieldClassDecl, FieldType);
      }
      else
        EmitClassMemberwiseCopy(LHS.getAddress(), RHS.getAddress(),
                                0 /*ClassDecl*/, FieldClassDecl, FieldType);
      continue;
    }
    // Do a built-in assignment of scalar data members.
    LValue LHS = EmitLValueForField(LoadOfThis, *Field, false, 0);
    LValue RHS = EmitLValueForField(LoadOfSrc, *Field, false, 0);
    if (!hasAggregateLLVMType(Field->getType())) {
      RValue RVRHS = EmitLoadOfLValue(RHS, Field->getType());
      EmitStoreThroughLValue(RVRHS, LHS, Field->getType());
    } else if (Field->getType()->isAnyComplexType()) {
      ComplexPairTy Pair = LoadComplexFromAddr(RHS.getAddress(),
                                               RHS.isVolatileQualified());
      StoreComplexToAddr(Pair, LHS.getAddress(), LHS.isVolatileQualified());
    } else {
      EmitAggregateCopy(LHS.getAddress(), RHS.getAddress(), Field->getType());
    }
  }
  FinishFunction();
}

/// SynthesizeCXXCopyAssignment - Implicitly define copy assignment operator.
/// Before the implicitly-declared copy assignment operator for a class is
/// implicitly defined, all implicitly- declared copy assignment operators for
/// its direct base classes and its nonstatic data members shall have been
/// implicitly defined. [12.8-p12]
/// The implicitly-defined copy assignment operator for class X performs
/// memberwise assignment of its subob- jects. The direct base classes of X are
/// assigned first, in the order of their declaration in
/// the base-specifier-list, and then the immediate nonstatic data members of X
/// are assigned, in the order in which they were declared in the class
/// definition.Each subobject is assigned in the manner appropriate to its type:
///   if the subobject is of class type, the copy assignment operator for the
///   class is used (as if by explicit qualification; that is, ignoring any
///   possible virtual overriding functions in more derived classes);
///
///   if the subobject is an array, each element is assigned, in the manner
///   appropriate to the element type;
///
///   if the subobject is of scalar type, the built-in assignment operator is
///   used.
void CodeGenFunction::SynthesizeCXXCopyAssignment(const CXXMethodDecl *CD,
                                                  llvm::Function *Fn,
                                                  const FunctionArgList &Args) {

  const CXXRecordDecl *ClassDecl = cast<CXXRecordDecl>(CD->getDeclContext());
  assert(!ClassDecl->hasUserDeclaredCopyAssignment() &&
         "SynthesizeCXXCopyAssignment - copy assignment has user declaration");
  StartFunction(CD, CD->getResultType(), Fn, Args, SourceLocation());

  FunctionArgList::const_iterator i = Args.begin();
  const VarDecl *ThisArg = i->first;
  llvm::Value *ThisObj = GetAddrOfLocalVar(ThisArg);
  llvm::Value *LoadOfThis = Builder.CreateLoad(ThisObj, "this");
  const VarDecl *SrcArg = (i+1)->first;
  llvm::Value *SrcObj = GetAddrOfLocalVar(SrcArg);
  llvm::Value *LoadOfSrc = Builder.CreateLoad(SrcObj);

  for (CXXRecordDecl::base_class_const_iterator Base = ClassDecl->bases_begin();
       Base != ClassDecl->bases_end(); ++Base) {
    // FIXME. copy assignment of virtual base NYI
    if (Base->isVirtual())
      continue;

    CXXRecordDecl *BaseClassDecl
      = cast<CXXRecordDecl>(Base->getType()->getAs<RecordType>()->getDecl());
    EmitClassCopyAssignment(LoadOfThis, LoadOfSrc, ClassDecl, BaseClassDecl,
                            Base->getType());
  }

  for (CXXRecordDecl::field_iterator Field = ClassDecl->field_begin(),
       FieldEnd = ClassDecl->field_end();
       Field != FieldEnd; ++Field) {
    QualType FieldType = getContext().getCanonicalType((*Field)->getType());
    const ConstantArrayType *Array =
      getContext().getAsConstantArrayType(FieldType);
    if (Array)
      FieldType = getContext().getBaseElementType(FieldType);

    if (const RecordType *FieldClassType = FieldType->getAs<RecordType>()) {
      CXXRecordDecl *FieldClassDecl
      = cast<CXXRecordDecl>(FieldClassType->getDecl());
      LValue LHS = EmitLValueForField(LoadOfThis, *Field, false, 0);
      LValue RHS = EmitLValueForField(LoadOfSrc, *Field, false, 0);
      if (Array) {
        const llvm::Type *BasePtr = ConvertType(FieldType);
        BasePtr = llvm::PointerType::getUnqual(BasePtr);
        llvm::Value *DestBaseAddrPtr =
          Builder.CreateBitCast(LHS.getAddress(), BasePtr);
        llvm::Value *SrcBaseAddrPtr =
          Builder.CreateBitCast(RHS.getAddress(), BasePtr);
        EmitClassAggrCopyAssignment(DestBaseAddrPtr, SrcBaseAddrPtr, Array,
                                    FieldClassDecl, FieldType);
      }
      else
        EmitClassCopyAssignment(LHS.getAddress(), RHS.getAddress(),
                               0 /*ClassDecl*/, FieldClassDecl, FieldType);
      continue;
    }
    // Do a built-in assignment of scalar data members.
    LValue LHS = EmitLValueForField(LoadOfThis, *Field, false, 0);
    LValue RHS = EmitLValueForField(LoadOfSrc, *Field, false, 0);
    RValue RVRHS = EmitLoadOfLValue(RHS, FieldType);
    EmitStoreThroughLValue(RVRHS, LHS, FieldType);
  }

  // return *this;
  Builder.CreateStore(LoadOfThis, ReturnValue);

  FinishFunction();
}

static void EmitBaseInitializer(CodeGenFunction &CGF, 
                                const CXXRecordDecl *ClassDecl,
                                CXXBaseOrMemberInitializer *BaseInit,
                                CXXCtorType CtorType) {
  assert(BaseInit->isBaseInitializer() &&
         "Must have base initializer!");

  llvm::Value *ThisPtr = CGF.LoadCXXThis();
  
  const Type *BaseType = BaseInit->getBaseClass();
  CXXRecordDecl *BaseClassDecl =
    cast<CXXRecordDecl>(BaseType->getAs<RecordType>()->getDecl());
  llvm::Value *V = CGF.GetAddressOfBaseClass(ThisPtr, ClassDecl,
                                             BaseClassDecl,
                                             /*NullCheckValue=*/false);
  CGF.EmitCXXConstructorCall(BaseInit->getConstructor(),
                             CtorType, V,
                             BaseInit->const_arg_begin(),
                             BaseInit->const_arg_end());
}

static void EmitMemberInitializer(CodeGenFunction &CGF,
                                  const CXXRecordDecl *ClassDecl,
                                  CXXBaseOrMemberInitializer *MemberInit) {
  assert(MemberInit->isMemberInitializer() &&
         "Must have member initializer!");
  
  // non-static data member initializers.
  FieldDecl *Field = MemberInit->getMember();
  QualType FieldType = CGF.getContext().getCanonicalType(Field->getType());

  llvm::Value *ThisPtr = CGF.LoadCXXThis();
  LValue LHS;
  if (FieldType->isReferenceType()) {
    // FIXME: This is really ugly; should be refactored somehow
    unsigned idx = CGF.CGM.getTypes().getLLVMFieldNo(Field);
    llvm::Value *V = CGF.Builder.CreateStructGEP(ThisPtr, idx, "tmp");
    assert(!FieldType.getObjCGCAttr() && "fields cannot have GC attrs");
    LHS = LValue::MakeAddr(V, CGF.MakeQualifiers(FieldType));
  } else {
    LHS = CGF.EmitLValueForField(ThisPtr, Field, ClassDecl->isUnion(), 0);
  }

  // If we are initializing an anonymous union field, drill down to the field.
  if (MemberInit->getAnonUnionMember()) {
    Field = MemberInit->getAnonUnionMember();
    LHS = CGF.EmitLValueForField(LHS.getAddress(), Field,
                                 /*IsUnion=*/true, 0);
    FieldType = Field->getType();
  }

  // If the field is an array, branch based on the element type.
  const ConstantArrayType *Array =
    CGF.getContext().getAsConstantArrayType(FieldType);
  if (Array)
    FieldType = CGF.getContext().getBaseElementType(FieldType);

  // We lose the constructor for anonymous union members, so handle them
  // explicitly.
  // FIXME: This is somwhat ugly.
  if (MemberInit->getAnonUnionMember() && FieldType->getAs<RecordType>()) {
    if (MemberInit->getNumArgs())
      CGF.EmitAggExpr(*MemberInit->arg_begin(), LHS.getAddress(),
                      LHS.isVolatileQualified());
    else
      CGF.EmitAggregateClear(LHS.getAddress(), Field->getType());
    return;
  }

  if (FieldType->getAs<RecordType>()) {
    assert(MemberInit->getConstructor() &&
           "EmitCtorPrologue - no constructor to initialize member");
    if (Array) {
      const llvm::Type *BasePtr = CGF.ConvertType(FieldType);
      BasePtr = llvm::PointerType::getUnqual(BasePtr);
      llvm::Value *BaseAddrPtr =
        CGF.Builder.CreateBitCast(LHS.getAddress(), BasePtr);
      CGF.EmitCXXAggrConstructorCall(MemberInit->getConstructor(),
                                     Array, BaseAddrPtr);
    }
    else
      CGF.EmitCXXConstructorCall(MemberInit->getConstructor(),
                                 Ctor_Complete, LHS.getAddress(),
                                 MemberInit->const_arg_begin(),
                                 MemberInit->const_arg_end());
    return;
  }

  assert(MemberInit->getNumArgs() == 1 && "Initializer count must be 1 only");
  Expr *RhsExpr = *MemberInit->arg_begin();
  RValue RHS;
  if (FieldType->isReferenceType()) {
    RHS = CGF.EmitReferenceBindingToExpr(RhsExpr, FieldType,
                                    /*IsInitializer=*/true);
    CGF.EmitStoreThroughLValue(RHS, LHS, FieldType);
  } else if (Array) {
    CGF.EmitMemSetToZero(LHS.getAddress(), Field->getType());
  } else if (!CGF.hasAggregateLLVMType(RhsExpr->getType())) {
    RHS = RValue::get(CGF.EmitScalarExpr(RhsExpr, true));
    CGF.EmitStoreThroughLValue(RHS, LHS, FieldType);
  } else if (RhsExpr->getType()->isAnyComplexType()) {
    CGF.EmitComplexExprIntoAddr(RhsExpr, LHS.getAddress(),
                                LHS.isVolatileQualified());
  } else {
    // Handle member function pointers; other aggregates shouldn't get this far.
    CGF.EmitAggExpr(RhsExpr, LHS.getAddress(), LHS.isVolatileQualified());
  }
}

/// EmitCtorPrologue - This routine generates necessary code to initialize
/// base classes and non-static data members belonging to this constructor.
/// FIXME: This needs to take a CXXCtorType.
void CodeGenFunction::EmitCtorPrologue(const CXXConstructorDecl *CD,
                                       CXXCtorType CtorType) {
  const CXXRecordDecl *ClassDecl = CD->getParent();
  
  // FIXME: Add vbase initialization
  llvm::Value *LoadOfThis = 0;

  for (CXXConstructorDecl::init_const_iterator B = CD->init_begin(),
       E = CD->init_end();
       B != E; ++B) {
    CXXBaseOrMemberInitializer *Member = (*B);
    
    assert(LiveTemporaries.empty() &&
           "Should not have any live temporaries at initializer start!");

    if (Member->isBaseInitializer())
      EmitBaseInitializer(*this, ClassDecl, Member, CtorType);
    else
      EmitMemberInitializer(*this, ClassDecl, Member);

    // Pop any live temporaries that the initializers might have pushed.
    while (!LiveTemporaries.empty())
      PopCXXTemporary();
  }

  // Initialize the vtable pointer
  if (ClassDecl->isDynamicClass()) {
    if (!LoadOfThis)
      LoadOfThis = LoadCXXThis();
    llvm::Value *VtableField;
    llvm::Type *Ptr8Ty, *PtrPtr8Ty;
    Ptr8Ty = llvm::PointerType::get(llvm::Type::getInt8Ty(VMContext), 0);
    PtrPtr8Ty = llvm::PointerType::get(Ptr8Ty, 0);
    VtableField = Builder.CreateBitCast(LoadOfThis, PtrPtr8Ty);
    llvm::Value *vtable = CGM.getVtableInfo().getVtable(ClassDecl);
    Builder.CreateStore(vtable, VtableField);
  }
}

/// EmitDtorEpilogue - Emit all code that comes at the end of class's
/// destructor. This is to call destructors on members and base classes
/// in reverse order of their construction.
/// FIXME: This needs to take a CXXDtorType.
void CodeGenFunction::EmitDtorEpilogue(const CXXDestructorDecl *DD,
                                       CXXDtorType DtorType) {
  assert(!DD->isTrivial() &&
         "Should not emit dtor epilogue for trivial dtor!");

  const CXXRecordDecl *ClassDecl = DD->getParent();

  // Collect the fields.
  llvm::SmallVector<const FieldDecl *, 16> FieldDecls;
  for (CXXRecordDecl::field_iterator I = ClassDecl->field_begin(),
       E = ClassDecl->field_end(); I != E; ++I) {
    const FieldDecl *Field = *I;
    
    QualType FieldType = getContext().getCanonicalType(Field->getType());
    FieldType = getContext().getBaseElementType(FieldType);
    
    const RecordType *RT = FieldType->getAs<RecordType>();
    if (!RT)
      continue;
    
    CXXRecordDecl *FieldClassDecl = cast<CXXRecordDecl>(RT->getDecl());
    if (FieldClassDecl->hasTrivialDestructor())
        continue;
    
    FieldDecls.push_back(Field);
  }
  
  // Now destroy the fields.
  for (size_t i = FieldDecls.size(); i > 0; --i) {
    const FieldDecl *Field = FieldDecls[i - 1];
    
    QualType FieldType = Field->getType();
    const ConstantArrayType *Array = 
      getContext().getAsConstantArrayType(FieldType);
    if (Array)
      FieldType = getContext().getBaseElementType(FieldType);
    
    const RecordType *RT = FieldType->getAs<RecordType>();
    CXXRecordDecl *FieldClassDecl = cast<CXXRecordDecl>(RT->getDecl());

    llvm::Value *ThisPtr = LoadCXXThis();

    LValue LHS = EmitLValueForField(ThisPtr, Field, 
                                    /*isUnion=*/false,
                                    // FIXME: Qualifiers?
                                    /*CVRQualifiers=*/0);
    if (Array) {
      const llvm::Type *BasePtr = ConvertType(FieldType);
      BasePtr = llvm::PointerType::getUnqual(BasePtr);
      llvm::Value *BaseAddrPtr =
        Builder.CreateBitCast(LHS.getAddress(), BasePtr);
      EmitCXXAggrDestructorCall(FieldClassDecl->getDestructor(getContext()),
                                Array, BaseAddrPtr);
    } else
      EmitCXXDestructorCall(FieldClassDecl->getDestructor(getContext()),
                            Dtor_Complete, LHS.getAddress());
  }

  // Destroy non-virtual bases.
  for (CXXRecordDecl::reverse_base_class_const_iterator I = 
        ClassDecl->bases_rbegin(), E = ClassDecl->bases_rend(); I != E; ++I) {
    const CXXBaseSpecifier &Base = *I;
    
    // Ignore virtual bases.
    if (Base.isVirtual())
      continue;
    
    CXXRecordDecl *BaseClassDecl
      = cast<CXXRecordDecl>(Base.getType()->getAs<RecordType>()->getDecl());
    
    // Ignore trivial destructors.
    if (BaseClassDecl->hasTrivialDestructor())
      continue;

    llvm::Value *V = GetAddressOfBaseClass(LoadCXXThis(),
                                           ClassDecl, BaseClassDecl, 
                                           /*NullCheckValue=*/false);
    EmitCXXDestructorCall(BaseClassDecl->getDestructor(getContext()),
                          Dtor_Base, V);
  }

  // If we're emitting a base destructor, we don't want to emit calls to the
  // virtual bases.
  if (DtorType == Dtor_Base)
    return;
  
  // FIXME: Handle virtual bases.
  for (CXXRecordDecl::reverse_base_class_const_iterator I = 
       ClassDecl->vbases_rbegin(), E = ClassDecl->vbases_rend(); I != E; ++I) {
    assert(false && "FIXME: Handle virtual bases.");
  }
    
  // If we have a deleting destructor, emit a call to the delete operator.
  if (DtorType == Dtor_Deleting)
    EmitDeleteCall(DD->getOperatorDelete(), LoadCXXThis(),
                   getContext().getTagDeclType(ClassDecl));
}

void CodeGenFunction::SynthesizeDefaultDestructor(const CXXDestructorDecl *Dtor,
                                                  CXXDtorType DtorType,
                                                  llvm::Function *Fn,
                                                  const FunctionArgList &Args) {
  assert(!Dtor->getParent()->hasUserDeclaredDestructor() &&
         "SynthesizeDefaultDestructor - destructor has user declaration");

  StartFunction(GlobalDecl(Dtor, DtorType), Dtor->getResultType(), Fn, Args, 
                SourceLocation());

  EmitDtorEpilogue(Dtor, DtorType);
  FinishFunction();
}

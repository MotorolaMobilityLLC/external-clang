LOCAL_PATH:= $(call my-dir)

clang_static_analyzer_checkers_TBLGEN_TABLES := \
  AttrKinds.inc \
  AttrList.inc \
  Attrs.inc \
  Checkers.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  StmtNodes.inc

clang_static_analyzer_checkers_SRC_FILES := \
  AdjustedReturnValueChecker.cpp \
  AnalyzerStatsChecker.cpp \
  ArrayBoundChecker.cpp \
  ArrayBoundCheckerV2.cpp \
  AttrNonNullChecker.cpp \
  BasicObjCFoundationChecks.cpp \
  BuiltinFunctionChecker.cpp \
  CStringChecker.cpp \
  CallAndMessageChecker.cpp \
  CastSizeChecker.cpp \
  CastToStructChecker.cpp \
  CheckObjCDealloc.cpp \
  CheckObjCInstMethSignature.cpp \
  CheckSecuritySyntaxOnly.cpp \
  CheckSizeofPointer.cpp \
  ChrootChecker.cpp \
  ClangSACheckerProvider.cpp \
  DeadStoresChecker.cpp \
  DebugCheckers.cpp \
  DereferenceChecker.cpp \
  DivZeroChecker.cpp \
  FixedAddressChecker.cpp \
  IdempotentOperationChecker.cpp \
  IteratorsChecker.cpp \
  LLVMConventionsChecker.cpp \
  MacOSXAPIChecker.cpp \
  MallocChecker.cpp \
  NSAutoreleasePoolChecker.cpp \
  NSErrorChecker.cpp \
  NoReturnFunctionChecker.cpp \
  OSAtomicChecker.cpp \
  ObjCAtSyncChecker.cpp \
  ObjCSelfInitChecker.cpp \
  ObjCUnusedIVarsChecker.cpp \
  PointerArithChecker.cpp \
  PointerSubChecker.cpp \
  PthreadLockChecker.cpp \
  ReturnPointerRangeChecker.cpp \
  ReturnUndefChecker.cpp \
  StackAddrEscapeChecker.cpp \
  StreamChecker.cpp \
  UndefBranchChecker.cpp \
  UndefCapturedBlockVarChecker.cpp \
  UndefResultChecker.cpp \
  UndefinedArraySubscriptChecker.cpp \
  UndefinedAssignmentChecker.cpp \
  UnixAPIChecker.cpp \
  UnreachableCodeChecker.cpp \
  VLASizeChecker.cpp

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := $(clang_static_analyzer_checkers_TBLGEN_TABLES)

LOCAL_SRC_FILES := $(clang_static_analyzer_checkers_SRC_FILES)

LOCAL_MODULE:= libclangStaticAnalyzerCheckers

LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

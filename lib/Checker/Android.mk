LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE:= libclangChecker

LOCAL_MODULE_TAGS := optional

TBLGEN_TABLES := \
  AttrList.inc \
  Attrs.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  StmtNodes.inc
#  AttrPCHRead.inc \
#  AttrPCHWrite.inc \
#  DiagnosticFrontendKinds.inc \
#  Attrs.inc \
#  Version.inc

clang_checker_SRC_FILES := \
  AdjustedReturnValueChecker.cpp \
  AggExprVisitor.cpp \
  AnalysisConsumer.cpp \
  AnalysisManager.cpp \
  AnalyzerStatsChecker.cpp \
  ArrayBoundChecker.cpp \
  AttrNonNullChecker.cpp \
  BasicConstraintManager.cpp \
  BasicObjCFoundationChecks.cpp \
  BasicStore.cpp \
  BasicValueFactory.cpp \
  BugReporter.cpp \
  BugReporterVisitors.cpp \
  BuiltinFunctionChecker.cpp \
  CFRefCount.cpp \
  CallAndMessageChecker.cpp \
  CastSizeChecker.cpp \
  CastToStructChecker.cpp \
  CheckDeadStores.cpp \
  CheckObjCDealloc.cpp \
  CheckObjCInstMethSignature.cpp \
  CheckSecuritySyntaxOnly.cpp \
  CheckSizeofPointer.cpp \
  Checker.cpp \
  CheckerHelpers.cpp \
  ChrootChecker.cpp \
  CocoaConventions.cpp \
  CStringChecker.cpp \
  DereferenceChecker.cpp \
  DivZeroChecker.cpp \
  Environment.cpp \
  ExplodedGraph.cpp \
  FixedAddressChecker.cpp \
  FlatStore.cpp \
  FrontendActions.cpp \
  GRBlockCounter.cpp \
  GRCXXExprEngine.cpp \
  GRCoreEngine.cpp \
  GRExprEngine.cpp \
  GRExprEngineExperimentalChecks.cpp \
  GRState.cpp \
  HTMLDiagnostics.cpp \
  IdempotentOperationChecker.cpp \
  LLVMConventionsChecker.cpp \
  MacOSXAPIChecker.cpp \
  MallocChecker.cpp \
  ManagerRegistry.cpp \
  MemRegion.cpp \
  NSAutoreleasePoolChecker.cpp \
  NSErrorChecker.cpp \
  NoReturnFunctionChecker.cpp \
  OSAtomicChecker.cpp \
  ObjCUnusedIVarsChecker.cpp \
  ObjCAtSyncChecker.cpp \
  PathDiagnostic.cpp \
  PlistDiagnostics.cpp \
  PointerArithChecker.cpp \
  PointerSubChecker.cpp \
  PthreadLockChecker.cpp \
  RangeConstraintManager.cpp \
  RegionStore.cpp \
  ReturnPointerRangeChecker.cpp \
  ReturnUndefChecker.cpp \
  SVals.cpp \
  SValuator.cpp \
  SimpleConstraintManager.cpp \
  SimpleSValuator.cpp \
  StackAddrLeakChecker.cpp \
  Store.cpp \
  StreamChecker.cpp \
  SymbolManager.cpp \
  UndefBranchChecker.cpp \
  UndefCapturedBlockVarChecker.cpp \
  UndefResultChecker.cpp \
  UndefinedArraySubscriptChecker.cpp \
  UndefinedAssignmentChecker.cpp \
  UnixAPIChecker.cpp \
  UnreachableCodeChecker.cpp \
  VLASizeChecker.cpp \
  ValueManager.cpp

LOCAL_SRC_FILES := $(clang_checker_SRC_FILES)


include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

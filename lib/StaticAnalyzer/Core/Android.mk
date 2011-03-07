LOCAL_PATH:= $(call my-dir)

clang_static_analyzer_core_TBLGEN_TABLES := \
  AttrList.inc \
  Attrs.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  StmtNodes.inc

clang_static_analyzer_core_SRC_FILES := \
  AggExprVisitor.cpp \
  AnalysisManager.cpp \
  BasicConstraintManager.cpp \
  BasicStore.cpp \
  BasicValueFactory.cpp \
  BugReporter.cpp \
  BugReporterVisitors.cpp \
  CFRefCount.cpp \
  CheckerContext.cpp \
  CheckerHelpers.cpp \
  CheckerManager.cpp \
  Environment.cpp \
  ExplodedGraph.cpp \
  ExprEngine.cpp \
  FlatStore.cpp \
  BlockCounter.cpp \
  CXXExprEngine.cpp \
  CoreEngine.cpp \
  GRState.cpp \
  HTMLDiagnostics.cpp \
  MemRegion.cpp \
  ObjCMessage.cpp \
  PathDiagnostic.cpp \
  PlistDiagnostics.cpp \
  RangeConstraintManager.cpp \
  RegionStore.cpp \
  SimpleConstraintManager.cpp \
  SimpleSValBuilder.cpp \
  Store.cpp \
  SValBuilder.cpp \
  SVals.cpp \
  SymbolManager.cpp \
  TextPathDiagnostics.cpp

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := $(clang_static_analyzer_core_TBLGEN_TABLES)

LOCAL_SRC_FILES := $(clang_static_analyzer_core_SRC_FILES)

LOCAL_MODULE:= libclangStaticAnalyzerCore

LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE:= libclangIndex

LOCAL_MODULE_TAGS := optional

TBLGEN_TABLES := \
  AttrList.inc \
  Attrs.inc \
  DiagnosticCommonKinds.inc \
  DeclNodes.inc \
  StmtNodes.inc

clang_index_SRC_FILES := \
  ASTLocation.cpp \
  Analyzer.cpp \
  CallGraph.cpp \
  DeclReferenceMap.cpp \
  Entity.cpp \
  GlobalSelector.cpp \
  Handlers.cpp \
  IndexProvider.cpp \
  Indexer.cpp \
  Program.cpp \
  SelectorMap.cpp

LOCAL_SRC_FILES := $(clang_index_SRC_FILES)


include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE:= libclangSerialization

LOCAL_MODULE_TAGS := optional

TBLGEN_TABLES := \
  AttrList.inc \
  AttrPCHRead.inc \
  AttrPCHWrite.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticFrontendKinds.inc \
  Attrs.inc \
  DeclNodes.inc \
  StmtNodes.inc

clang_serialization_SRC_FILES :=\
  GeneratePCH.cpp \
  ASTCommon.cpp \
  ASTReader.cpp \
  ASTReaderDecl.cpp \
  ASTReaderStmt.cpp \
  ASTWriter.cpp \
  ASTWriterDecl.cpp \
  ASTWriterStmt.cpp \
  ChainedIncludesSource.cpp

LOCAL_SRC_FILES := $(clang_serialization_SRC_FILES)


include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(CLANG_VERSION_INC_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

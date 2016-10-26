LOCAL_PATH:= $(call my-dir)

clang_static_analyzer_checkers_TBLGEN_TABLES := \
  AttrKinds.inc \
  AttrList.inc \
  AttrParsedAttrList.inc \
  Attrs.inc \
  AttrVisitor.inc \
  Checkers.inc \
  CommentCommandList.inc \
  CommentNodes.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  StmtNodes.inc

clang_static_analyzer_checkers_SRC_FILES := $(sort $(notdir $(wildcard $(LOCAL_PATH)/*.cpp)))

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
include $(CLANG_VERSION_INC_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

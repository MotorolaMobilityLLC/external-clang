LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \

clang_tooling_core_SRC_FILES := \
  Lookup.cpp \
  Replacement.cpp \

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_tooling_core_SRC_FILES)
LOCAL_MODULE:= libclangToolingCore
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

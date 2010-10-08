LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES :=	\
	DiagnosticCommonKinds.inc	\
	DiagnosticDriverKinds.inc

clang_driver_SRC_FILES :=	\
	Arg.cpp	\
	ArgList.cpp	\
	Option.cpp	\
	OptTable.cpp

LOCAL_SRC_FILES := $(clang_driver_SRC_FILES)

LOCAL_MODULE := libclangDriver
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

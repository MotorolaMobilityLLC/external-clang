LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES :=

clang_SRC_FILES := \
    cc1_main.cpp \
    cc1as_main.cpp \
    driver.cpp

LOCAL_SRC_FILES := $(clang_SRC_FILES)

LOCAL_MODULE := clang
LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_LIBRARIES := \
		libclangFrontendTool \
		libclangFrontend \
		libclangDriver \
		libclangSerialization \
		libclangCodeGen \
		libclangParse \
		libclangSema \
		libclangChecker \
		libclangAnalysis \
		libclangIndex \
		libclangRewrite \
		libclangAST \
		libclangLex \
		libclangBasic \
		libLLVMSelectionDAG \
		libLLVMipo \
		libLLVMCodeGen \
		libLLVMScalarOpts \
		libLLVMInstCombine \
		libLLVMTransformUtils \
		libLLVMipa \
		libLLVMAnalysis \
		libLLVMTarget \
		libLLVMMC \
		libLLVMBitWriter \
		libLLVMBitReader \
		libLLVMAsmParser \
		libLLVMCore \
		libLLVMSupport \
		libLLVMSystem

LOCAL_CPPFLAGS += -fno-rtti -fno-exceptions

LOCAL_LDFLAGS += -lpthread -lm -ldl

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_EXECUTABLE)

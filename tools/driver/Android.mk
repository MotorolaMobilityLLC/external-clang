LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE := clang

LOCAL_MODULE_CLASS := EXECUTABLES

TBLGEN_TABLES := \
  DiagnosticCommonKinds.inc \
  DiagnosticDriverKinds.inc \
  DiagnosticFrontendKinds.inc \
  CC1Options.inc \
  CC1AsOptions.inc \
  Options.inc

clang_SRC_FILES := \
  cc1_main.cpp \
  cc1as_main.cpp \
  driver.cpp

LOCAL_SRC_FILES := $(clang_SRC_FILES)

LOCAL_STATIC_LIBRARIES := \
  libclangFrontendTool \
  libclangFrontend \
  libclangARCMigrate \
  libclangDriver \
  libclangSerialization \
  libclangCodeGen \
  libclangRewriteFrontend \
  libclangRewrite \
  libclangParse \
  libclangSema \
  libclangStaticAnalyzerFrontend \
  libclangStaticAnalyzerCheckers \
  libclangStaticAnalyzerCore \
  libclangAnalysis \
  libclangEdit \
  libclangAST \
  libclangLex \
  libclangBasic \
  libLLVMARMAsmParser \
  libLLVMARMCodeGen \
  libLLVMARMAsmPrinter \
  libLLVMARMDisassembler \
  libLLVMARMDesc \
  libLLVMARMInfo \
  libLLVMMipsAsmParser \
  libLLVMMipsCodeGen \
  libLLVMMipsDisassembler \
  libLLVMMipsAsmPrinter \
  libLLVMMipsDesc \
  libLLVMMipsInfo \
  libLLVMX86Info \
  libLLVMX86AsmParser \
  libLLVMX86CodeGen \
  libLLVMX86Disassembler \
  libLLVMX86Desc \
  libLLVMX86AsmPrinter \
  libLLVMX86Utils \
  libLLVMAArch64Info \
  libLLVMAArch64AsmParser \
  libLLVMAArch64CodeGen \
  libLLVMAArch64Disassembler \
  libLLVMAArch64Desc \
  libLLVMAArch64AsmPrinter \
  libLLVMAArch64Utils \
  libLLVMIRReader \
  libLLVMAsmParser \
  libLLVMAsmPrinter \
  libLLVMBitReader \
  libLLVMBitWriter \
  libLLVMSelectionDAG \
  libLLVMipo \
  libLLVMInstCombine \
  libLLVMInstrumentation \
  libLLVMCodeGen \
  libLLVMObject \
  libLLVMLinker \
  libLLVMMC \
  libLLVMMCParser \
  libLLVMScalarOpts \
  libLLVMTransformObjCARC \
  libLLVMTransformUtils \
  libLLVMVectorize \
  libLLVMAnalysis \
  libLLVMCore \
  libLLVMOption \
  libLLVMTarget \
  libLLVMProfileData \
  libLLVMObject \
  libLLVMMCDisassembler \
  libLLVMSupport

LOCAL_LDLIBS += -lm
LOCAL_LDLIBS_windows := -limagehlp
LOCAL_LDLIBS_darwin := -lpthread -ldl
LOCAL_LDLIBS_linux := -lpthread -ldl

# remove when we can use PIE binaries in all places again
LOCAL_NO_FPIE := true

# Create symlink clang++ pointing to clang.
# Use "=" (instead of ":=") to defer the evaluation.
LOCAL_POST_INSTALL_CMD = \
  $(hide) ln -sf clang$(LOCAL_MODULE_SUFFIX) \
  $(dir $(LOCAL_INSTALLED_MODULE))clang++$(LOCAL_MODULE_SUFFIX)

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)

# Don't build both versions by default (to speed up local builds).
ifeq (true,$(FORCE_BUILD_LLVM_COMPONENTS))
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE)_32
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE)
endif

include $(BUILD_HOST_EXECUTABLE)

LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES :=    \
	AttrList.inc	\
	Attrs.inc	\
	DeclNodes.inc	\
	DiagnosticCommonKinds.inc	\
	DiagnosticAnalysisKinds.inc	\
	StmtNodes.inc

clang_analysis_SRC_FILES :=	\
	AnalysisContext.cpp	\
	CFG.cpp	\
	CFGReachabilityAnalysis.cpp	\
	CFGStmtMap.cpp	\
	CocoaConventions.cpp	\
	FormatString.cpp	\
	LiveVariables.cpp	\
	PrintfFormatString.cpp	\
	PseudoConstantAnalysis.cpp	\
	ReachableCode.cpp	\
	ScanfFormatString.cpp	\
	UninitializedValues.cpp


LOCAL_SRC_FILES := $(clang_analysis_SRC_FILES)

LOCAL_MODULE:= libclangAnalysis
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

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
	DiagnosticFrontendKinds.inc	\
	StmtNodes.inc	\
	arm_neon.inc

clang_codegen_SRC_FILES :=	\
	BackendUtil.cpp	\
	CGBlocks.cpp	\
	CGBuiltin.cpp	\
	CGCall.cpp	\
	CGClass.cpp	\
	CGCUDANV.cpp \
	CGCUDARuntime.cpp \
	CGOpenCLRuntime.cpp \
	CGCXX.cpp	\
	CGCXXABI.cpp	\
	CGCleanup.cpp	\
	CGDebugInfo.cpp	\
	CGDecl.cpp	\
	CGDeclCXX.cpp	\
	CGException.cpp	\
	CGExpr.cpp	\
	CGExprAgg.cpp	\
	CGExprComplex.cpp	\
	CGExprConstant.cpp	\
	CGExprCXX.cpp	\
	CGExprScalar.cpp	\
	CGObjC.cpp	\
	CGObjCGNU.cpp	\
	CGObjCMac.cpp	\
	CGObjCRuntime.cpp \
	CGRecordLayoutBuilder.cpp	\
	CGRTTI.cpp	\
	CGStmt.cpp	\
	CGTemporaries.cpp	\
	CGVTables.cpp	\
	CGVTT.cpp	\
	CodeGenAction.cpp	\
	CodeGenFunction.cpp	\
	CodeGenModule.cpp	\
	CodeGenTBAA.cpp	\
	CodeGenTypes.cpp	\
	ItaniumCXXABI.cpp	\
	MicrosoftCXXABI.cpp	\
	ModuleBuilder.cpp	\
	TargetInfo.cpp

LOCAL_SRC_FILES := $(clang_codegen_SRC_FILES)

LOCAL_MODULE:= libclangCodeGen

LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

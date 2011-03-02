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
	CGCXX.cpp	\
	CGCall.cpp	\
	CGClass.cpp	\
	CGDebugInfo.cpp	\
	CGDecl.cpp	\
	CGDeclCXX.cpp	\
	CGException.cpp	\
	CGExpr.cpp	\
	CGExprAgg.cpp	\
	CGExprCXX.cpp	\
	CGExprComplex.cpp	\
	CGExprConstant.cpp	\
	CGExprScalar.cpp	\
	CGObjC.cpp	\
	CGObjCGNU.cpp	\
	CGObjCMac.cpp	\
	CGRTTI.cpp	\
	CGRecordLayoutBuilder.cpp	\
	CGStmt.cpp	\
	CGTemporaries.cpp	\
	CGVTT.cpp	\
	CGVTables.cpp	\
	CodeGenAction.cpp	\
	CodeGenFunction.cpp	\
	CodeGenModule.cpp	\
	CodeGenTBAA.cpp	\
	CodeGenTypes.cpp	\
	ItaniumCXXABI.cpp	\
	Mangle.cpp	\
	MicrosoftCXXABI.cpp	\
	ModuleBuilder.cpp	\
	TargetInfo.cpp

LOCAL_SRC_FILES := $(clang_codegen_SRC_FILES)

LOCAL_MODULE:= libclangCodeGen
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

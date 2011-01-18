LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES :=	

clang_frontend_SRC_FILES :=	\
		AdjustedReturnValueChecker.cpp  \
		CheckerHelpers.cpp             \
		GRExprEngineInternalChecks.h    \
		RegionStore.cpp \
		AggExprVisitor.cpp              \
		CheckObjCDealloc.cpp      \
		GRState.cpp                  \
		ReturnPointerRangeChecker.cpp \
		AnalysisConsumer.cpp           \
		CheckObjCInstMethSignature.cpp      \
		HTMLDiagnostics.cpp             \
		ReturnUndefChecker.cpp \
		AnalysisManager.cpp             \
		CheckSecuritySyntaxOnly.cpp   \
 		IdempotentOperationChecker.cpp \
	 	SimpleConstraintManager.cpp\
    CheckSizeofPointer.cpp     \
		LLVMConventionsChecker.cpp   \
 		SimpleConstraintManager.h\
		ArrayBoundChecker.cpp    \
		MacOSXAPIChecker.cpp     \
		SimpleSValuator.cpp\
		AttrNonNullChecker.cpp     \
		CocoaConventions.cpp       \
    StackAddrLeakChecker.cpp\
		BasicConstraintManager.cpp      \
		CStringChecker.cpp         \
		MallocChecker.cpp         \
		Store.cpp\
		BasicObjCFoundationChecks.cpp  \
	 	DereferenceChecker.cpp       \
		ManagerRegistry.cpp      \
		StreamChecker.cpp\
		BasicObjCFoundationChecks.h  DivZeroChecker.cpp                  MemRegion.cpp                   SVals.cpp \
		BasicStore.cpp                  Environment.cpp                     NoReturnFunctionChecker.cpp     SValuator.cpp \
		BasicValueFactory.cpp           ExplodedGraph.cpp                   NSAutoreleasePoolChecker.cpp    SymbolManager.cpp \
		BugReporter.cpp                 FixedAddressChecker.cpp             NSErrorChecker.cpp              UndefBranchChecker.cpp \
		BugReporterVisitors.cpp         FlatStore.cpp                       ObjCUnusedIVarsChecker.cpp      UndefCapturedBlockVarChecker.cpp \
		BuiltinFunctionChecker.cpp      FrontendActions.cpp                 OSAtomicChecker.cpp             UndefinedArraySubscriptChecker.cpp \
		CallAndMessageChecker.cpp       GRBlockCounter.cpp                  PathDiagnostic.cpp              UndefinedAssignmentChecker.cpp \
		CastSizeChecker.cpp             GRCoreEngine.cpp                    PlistDiagnostics.cpp            UndefResultChecker.cpp \
		CastToStructChecker.cpp         GRCXXExprEngine.cpp                 PointerArithChecker.cpp         UnixAPIChecker.cpp \
		CFRefCount.cpp                  GRExprEngine.cpp                    PointerSubChecker.cpp           UnreachableCodeChecker.cpp \
		CheckDeadStores.cpp             GRExprEngineExperimentalChecks.cpp  PthreadLockChecker.cpp          ValueManager.cpp \
		Checker.cpp                     GRExprEngineExperimentalChecks.h    RangeConstraintManager.cpp      VLASizeChecker.cpp

LOCAL_SRC_FILES := $(clang_frontend_SRC_FILES)

LOCAL_MODULE:= libclangChecker

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

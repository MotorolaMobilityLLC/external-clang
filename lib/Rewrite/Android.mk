LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES :=	

clang_frontend_SRC_FILES :=	\
		DeltaTree.cpp      FrontendActions.cpp  HTMLRewrite.cpp  RewriteMacros.cpp  Rewriter.cpp     RewriteTest.cpp \
		FixItRewriter.cpp  HTMLPrint.cpp        RewriteObjC.cpp  RewriteRope.cpp  TokenRewriter.cpp



LOCAL_SRC_FILES := $(clang_frontend_SRC_FILES)

LOCAL_MODULE:= libclangRewrite

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

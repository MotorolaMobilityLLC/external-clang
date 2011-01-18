LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES :=	

clang_frontend_SRC_FILES :=	\
	ASTCommon.h \
	ASTReaderDecl.cpp \
	ASTWriter.cpp \
	ASTWriterStmt.cpp \
	GeneratePCH.cpp \
	ASTCommon.cpp \
	ASTReader.cpp \
	ASTReaderStmt.cpp \
	ASTWriterDecl.cpp

LOCAL_SRC_FILES := $(clang_frontend_SRC_FILES)

LOCAL_MODULE:= libclangSerialization

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

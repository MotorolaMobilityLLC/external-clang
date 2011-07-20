LOCAL_PATH := $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  Attrs.inc \
  AttrList.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticGroups.inc \
  DiagnosticSemaKinds.inc \
  StmtNodes.inc

clang_arc_migrate_SRC_FILES := \
  ARCMT.cpp \
  ARCMTActions.cpp \
  FileRemapper.cpp \
  PlistReporter.cpp \
  TransAPIUses.cpp \
  TransARCAssign.cpp \
  TransAutoreleasePool.cpp \
  TransBlockObjCVariable.cpp \
  TransEmptyStatementsAndDealloc.cpp \
  TransformActions.cpp \
  Transforms.cpp \
  TransProperties.cpp \
  TransRetainReleaseDealloc.cpp \
  TransUnbridgedCasts.cpp \
  TransUnusedInitDelegate.cpp \
  TransZeroOutPropsInDealloc.cpp

LOCAL_SRC_FILES := $(clang_arc_migrate_SRC_FILES)

LOCAL_MODULE := libclangARCMigrate
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

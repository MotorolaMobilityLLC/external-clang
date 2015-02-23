LOCAL_PATH := $(call my-dir)
CLANG_ROOT_PATH := $(LOCAL_PATH)
CLANG_ENABLE_ASSERTION := false

include $(CLEAR_VARS)

subdirs := $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, \
  lib/Analysis \
  lib/AST \
  lib/ARCMigrate \
  lib/Basic \
  lib/CodeGen \
  lib/Driver \
  lib/Edit \
  lib/Format \
  lib/Frontend \
  lib/Frontend/Rewrite \
  lib/FrontendTool \
  lib/Headers \
  lib/Lex \
  lib/Parse \
  lib/Rewrite \
  lib/Sema \
  lib/Serialization \
  lib/StaticAnalyzer/Checkers \
  lib/StaticAnalyzer/Core \
  lib/StaticAnalyzer/Frontend \
  tools/driver \
  utils/TableGen \
  ))

include $(LOCAL_PATH)/clang.mk
include $(LOCAL_PATH)/shared_clang.mk

include $(subdirs)

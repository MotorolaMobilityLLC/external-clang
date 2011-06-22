LOCAL_PATH := $(call my-dir)
CLANG_ROOT_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

subdirs := $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, \
  lib/Analysis \
  lib/AST \
  lib/ARCMigrate \
  lib/Basic \
  lib/CodeGen \
  lib/Driver \
  lib/Frontend \
  lib/FrontendTool \
  lib/Index \
  lib/Lex \
  lib/Parse \
  lib/Rewrite \
  lib/Sema \
  lib/Serialization \
  lib/StaticAnalyzer/Checkers \
  lib/StaticAnalyzer/Core \
  lib/StaticAnalyzer/Frontend \
  tools/driver \
  ))

include $(LOCAL_PATH)/clang.mk

include $(subdirs)

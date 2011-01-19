LOCAL_PATH := $(call my-dir)
CLANG_ROOT_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

subdirs := $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, \
  lib/Analysis \
  lib/AST \
  lib/Basic \
  lib/Checker \
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
  tools/driver \
  ))

include $(LOCAL_PATH)/clang.mk

include $(subdirs)

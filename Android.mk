LOCAL_PATH := $(call my-dir)
CLANG_ROOT_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

subdirs := $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, \
		lib/Basic	\
		lib/Lex	\
		lib/Parse	\
		lib/AST	\
		lib/Sema	\
		lib/CodeGen	\
		lib/Analysis	\
	))

include $(LOCAL_PATH)/clang.mk

include $(subdirs)

LOCAL_CFLAGS :=	\
	-pedantic	\
	-Wcast-qual	\
	-Wno-long-long	\
	$(LOCAL_CFLAGS)

LOCAL_CPPFLAGS := \
	-Wno-sign-promo \
	$(LOCAL_CPPFLAGS)

# Make sure bionic is first so we can include system headers.
LOCAL_C_INCLUDES :=	\
	$(CLANG_ROOT_PATH)/include	\
        $(CLANG_ROOT_PATH)/lib/CodeGen    \
	$(LOCAL_C_INCLUDES)

LLVM_ROOT_PATH := external/llvm
include $(LLVM_ROOT_PATH)/llvm.mk

ifneq ($(LLVM_HOST_BUILD_MK),)
include $(LLVM_HOST_BUILD_MK)
endif

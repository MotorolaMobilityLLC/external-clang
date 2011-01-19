LOCAL_CFLAGS :=	\
	-pedantic	\
	-Wcast-qual	\
	-Wno-long-long	\
	$(LOCAL_CFLAGS)

# Make sure bionic is first so we can include system headers.
LOCAL_C_INCLUDES :=	\
	$(CLANG_ROOT_PATH)/include	\
	$(LOCAL_C_INCLUDES)

LLVM_ROOT_PATH := external/llvm
include $(LLVM_ROOT_PATH)/llvm.mk

ifneq ($(LLVM_HOST_BUILD_MK),)
include $(LLVM_HOST_BUILD_MK)
endif

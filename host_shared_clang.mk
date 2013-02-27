LOCAL_PATH:= $(call my-dir)

LOCAL_IS_HOST_MODULE := true

LOCAL_MODULE:= libclang

LOCAL_MODULE_TAGS := optional

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libclangDriver \
	libclangParse \
	libclangSema \
	libclangAnalysis \
	libclangCodeGen \
	libclangAST \
	libclangEdit \
	libclangLex \
	libclangFrontend \
	libclangBasic \
	libclangSerialization

LOCAL_SHARED_LIBRARIES := libLLVM

LOCAL_LDLIBS := -ldl -lpthread

include $(CLANG_HOST_BUILD_MK)
include $(BUILD_HOST_SHARED_LIBRARY)

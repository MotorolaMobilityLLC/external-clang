###########################################################
## Generate clang/Basic/Version.inc
###########################################################
ifeq ($(LOCAL_MODULE_CLASS),)
    LOCAL_MODULE_CLASS := STATIC_LIBRARIES
endif

intermediates := $(call local-generated-sources-dir)

# Compute the Clang version from the LLVM version, unless specified explicitly.
# (from include/clang/Basic/Makefile)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Basic/Version.inc
$(intermediates)/include/clang/Basic/Version.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Version.inc.in \
  $(CLANG_ROOT_PATH)/clang-version-inc.py $(CLANG_ROOT_PATH)/version.py
	@echo "Updating Clang version info."
	@mkdir -p $(dir $@)
	$(hide) $(CLANG_ROOT_PATH)/clang-version-inc.py $< >$@

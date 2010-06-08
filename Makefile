##===- Makefile --------------------------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

# If CLANG_LEVEL is not set, then we are the top-level Makefile. Otherwise, we
# are being included from a subdirectory makefile.

ifndef CLANG_LEVEL

IS_TOP_LEVEL := 1
CLANG_LEVEL := .
DIRS := include lib tools docs

PARALLEL_DIRS :=

ifeq ($(BUILD_EXAMPLES),1)
  PARALLEL_DIRS += examples
endif
endif

###
# Common Makefile code, shared by all Clang Makefiles.

# Set LLVM source root level.
LEVEL := $(CLANG_LEVEL)/../..

# Include LLVM common makefile.
include $(LEVEL)/Makefile.common

# Set common Clang build flags.
CPP.Flags += -I$(PROJ_SRC_DIR)/$(CLANG_LEVEL)/include -I$(PROJ_OBJ_DIR)/$(CLANG_LEVEL)/include
ifdef CLANG_VENDOR
CPP.Flags += -DCLANG_VENDOR='"$(CLANG_VENDOR) "'
endif

###
# Clang Top Level specific stuff.

ifeq ($(IS_TOP_LEVEL),1)

ifneq ($(PROJ_SRC_ROOT),$(PROJ_OBJ_ROOT))
$(RecursiveTargets)::
	$(Verb) if [ ! -f test/Makefile ]; then \
	  $(MKDIR) test; \
	  $(CP) $(PROJ_SRC_DIR)/test/Makefile test/Makefile; \
	fi
endif

test::
	@ $(MAKE) -C test

report::
	@ $(MAKE) -C test report

clean::
	@ $(MAKE) -C test clean

tags::
	$(Verb) etags `find . -type f -name '*.h' -or -name '*.cpp' | \
	  grep -v /lib/Headers | grep -v /test/`

cscope.files:
	find tools lib include -name '*.cpp' \
	                    -or -name '*.def' \
	                    -or -name '*.td' \
	                    -or -name '*.h' > cscope.files

.PHONY: test report clean cscope.files

endif

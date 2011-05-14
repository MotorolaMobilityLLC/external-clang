###########################################################
## TableGen: Compile .td files to .inc.
###########################################################
ifeq ($(LOCAL_MODULE_CLASS),)
    LOCAL_MODULE_CLASS := STATIC_LIBRARIES
endif

ifneq ($(strip $(TBLGEN_TABLES)),)

intermediates := $(call local-intermediates-dir)

ifneq ($(findstring AttrImpl.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/AST/AttrImpl.inc
$(intermediates)/include/clang/AST/AttrImpl.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(TBLGEN)
	@echo "Building Clang attribute implementations with tblgen"
	$(call transform-host-td-to-out,clang-attr-impl)
endif

ifneq ($(findstring AttrList.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Basic/AttrList.inc
$(intermediates)/include/clang/Basic/AttrList.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(TBLGEN)
	@echo "Building Clang attribute list with tblgen"
	$(call transform-host-td-to-out,clang-attr-list)
endif

ifneq ($(findstring AttrSpellings.inc,$(TBLGEN_TABLES)),)
    LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Lex/AttrSpellings.inc
$(intermediates)/include/clang/Lex/AttrSpellings.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(TBLGEN)
	@echo "Building Clang attribute spelling list with tblgen"
	$(call transform-host-td-to-out,clang-attr-spelling-list)
endif

ifneq ($(findstring AttrPCHRead.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Serialization/AttrPCHRead.inc
$(intermediates)/include/clang/Serialization/AttrPCHRead.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(TBLGEN)
	@echo "Building Clang attribute pch read with tblgen"
	$(call transform-host-td-to-out,clang-attr-pch-read)
endif

ifneq ($(findstring AttrPCHWrite.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Serialization/AttrPCHWrite.inc
$(intermediates)/include/clang/Serialization/AttrPCHWrite.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(TBLGEN)
	@echo "Building Clang attribute pch write with tblgen"
	$(call transform-host-td-to-out,clang-attr-pch-write)
endif

ifneq ($(findstring Attrs.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/AST/Attrs.inc
$(intermediates)/include/clang/AST/Attrs.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(TBLGEN)
	@echo "Building Clang attribute classes with tblgen"
	$(call transform-host-td-to-out,clang-attr-classes)
endif

ifneq ($(findstring Checkers.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/Checkers.inc
$(intermediates)/Checkers.inc: \
  $(CLANG_ROOT_PATH)/lib/StaticAnalyzer/Checkers/Checkers.td \
  $(CLANG_ROOT_PATH)/include/clang/StaticAnalyzer/Checkers/CheckerBase.td \
  $(TBLGEN)
	@echo "Building Clang static analyzer checkers list with tblgen"
	$(call transform-host-td-to-out,clang-sa-checkers)
endif

ifneq ($(filter Diagnostic%Kinds.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(addprefix $(intermediates)/include/clang/Basic/,$(filter Diagnostic%Kinds.inc,$(TBLGEN_TABLES)))
$(intermediates)/include/clang/Basic/Diagnostic%Kinds.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td $(TBLGEN)
	@echo "Building Clang $(patsubst Diagnostic%Kinds.inc,%,$(@F)) diagnostic tables with tblgen"
	$(call transform-host-td-to-out,clang-diags-defs -clang-component=$(patsubst Diagnostic%Kinds.inc,%,$(@F)))
endif

ifneq ($(findstring DiagnosticGroups.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Basic/DiagnosticGroups.inc
$(intermediates)/include/clang/Basic/DiagnosticGroups.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td $(CLANG_ROOT_PATH)/include/clang/Basic/DiagnosticGroups.td $(TBLGEN)
	@echo "Building Clang diagnostic groups with tblgen"
	$(call transform-host-td-to-out,clang-diag-groups)
endif

ifneq ($(findstring DiagnosticIndexName.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Basic/DiagnosticIndexName.inc
$(intermediates)/include/clang/Basic/DiagnosticIndexName.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td $(TBLGEN)
	@echo "Building Clang diagnostic name index with tblgen"
	$(call transform-host-td-to-out,clang-diag-groups)
endif

ifneq ($(findstring DeclNodes.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/AST/DeclNodes.inc
$(intermediates)/include/clang/AST/DeclNodes.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/DeclNodes.td $(TBLGEN)
	@echo "Building Clang declaration node tables with tblgen"
	$(call transform-host-td-to-out,clang-decl-nodes)
endif

ifneq ($(findstring StmtNodes.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/AST/StmtNodes.inc
$(intermediates)/include/clang/AST/StmtNodes.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/StmtNodes.td $(TBLGEN)
	@echo "Building Clang statement node tables with tblgen"
	$(call transform-host-td-to-out,clang-stmt-nodes)
endif

ifneq ($(findstring arm_neon.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Basic/arm_neon.inc
$(intermediates)/include/clang/Basic/arm_neon.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/arm_neon.td $(TBLGEN)
	@echo "Building Clang arm_neon.inc with tblgen"
	$(call transform-host-td-to-out,arm-neon-sema)
endif

ifneq ($(findstring Options.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Driver/Options.inc
$(intermediates)/include/clang/Driver/Options.inc: $(CLANG_ROOT_PATH)/include/clang/Driver/Options.td $(CLANG_ROOT_PATH)/include/clang/Driver/OptParser.td $(TBLGEN)
	@echo "Building Clang Driver Option tables with tblgen"
	$(call transform-host-td-to-out,opt-parser-defs)
endif

ifneq ($(findstring CC1Options.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Driver/CC1Options.inc
$(intermediates)/include/clang/Driver/CC1Options.inc: $(CLANG_ROOT_PATH)/include/clang/Driver/CC1Options.td $(CLANG_ROOT_PATH)/include/clang/Driver/OptParser.td $(TBLGEN)
	@echo "Building Clang CC1 Option tables with tblgen"
	$(call transform-host-td-to-out,opt-parser-defs)
endif

ifneq ($(findstring CC1AsOptions.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(intermediates)/include/clang/Driver/CC1AsOptions.inc
$(intermediates)/include/clang/Driver/CC1AsOptions.inc: $(CLANG_ROOT_PATH)/include/clang/Driver/CC1AsOptions.td $(CLANG_ROOT_PATH)/include/clang/Driver/OptParser.td $(TBLGEN)
	@echo "Building Clang CC1 As Option tables with tblgen"
	$(call transform-host-td-to-out,opt-parser-defs)
endif

LOCAL_C_INCLUDES += $(intermediates)/include

endif

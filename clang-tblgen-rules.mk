###########################################################
## Commands for running LLVM tblgen to compile a td file
##########################################################
define transform-llvm-td-to-out
$(if $(LOCAL_IS_HOST_MODULE),	\
	$(call transform-host-td-to-out,$(1)),	\
	$(call transform-device-td-to-out,$(1)))
endef

# $(1): an output file
# $(2): an input .td file
# $(3): a parameter passed to transform-td-to-out
# You must call this with $(eval).
define define-llvm-tblgen-rule
LOCAL_GENERATED_SOURCES += $(1)
$(1): TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(1): $(2) $(LLVM_TBLGEN)
	$$(call transform-llvm-td-to-out,$(3))
$$(call include-depfile, $(1).d, $(1))
endef

###################################4########################
## TableGen: Compile .td files to .inc.
###########################################################
ifeq ($(LOCAL_MODULE_CLASS),)
    LOCAL_MODULE_CLASS := STATIC_LIBRARIES
endif

ifneq ($(strip $(TBLGEN_TABLES)),)

define transform-clang-td-to-out
$(if $(LOCAL_IS_HOST_MODULE),	\
	$(call transform-host-clang-td-to-out,$(1)),	\
	$(call transform-device-clang-td-to-out,$(1)))
endef

# $(1): an output file
# $(2): an input .td file
# $(3): a parameter passed to transform-td-to-out
# You must call this with $(eval).
define define-clang-tblgen-rule
LOCAL_GENERATED_SOURCES += $(1)
$(1): TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(1): $(2) $(CLANG_TBLGEN)
	$$(call transform-clang-td-to-out,$(3))
$$(call include-depfile, $(1).d, $(1))
endef

generated_sources := $(call local-generated-sources-dir)

ifneq ($(filter AttrDump.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/AttrDump.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-dump))
endif

ifneq ($(filter AttrImpl.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/AttrImpl.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-impl))
endif

ifneq ($(filter AttrHasAttributeImpl.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/AttrHasAttributeImpl.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-has-attribute-impl))
endif

ifneq ($(filter AttrList.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/AttrList.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-list))
endif

ifneq ($(filter AttrSpellingListIndex.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Sema/AttrSpellingListIndex.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-spelling-index))
endif

ifneq ($(filter AttrPCHRead.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Serialization/AttrPCHRead.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-pch-read))
endif

ifneq ($(filter AttrPCHWrite.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Serialization/AttrPCHWrite.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-pch-write))
endif

ifneq ($(filter Attrs.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/Attrs.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-classes))
endif

ifneq ($(filter AttrParserStringSwitches.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Parse/AttrParserStringSwitches.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-parser-string-switches))
endif

ifneq ($(filter AttrVisitor.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/AttrVisitor.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-ast-visitor))
endif

ifneq ($(filter AttrParsedAttrKinds.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Sema/AttrParsedAttrKinds.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-parsed-attr-kinds))
endif

ifneq ($(filter AttrParsedAttrImpl.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Sema/AttrParsedAttrImpl.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-parsed-attr-impl))
endif

ifneq ($(filter AttrParsedAttrList.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Sema/AttrParsedAttrList.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-parsed-attr-list))
endif

ifneq ($(filter AttrTemplateInstantiate.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Sema/AttrTemplateInstantiate.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-template-instantiate))
endif

ifneq ($(filter Checkers.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/StaticAnalyzer/Checkers/Checkers.inc, \
    $(CLANG_ROOT_PATH)/include/clang/StaticAnalyzer/Checkers/Checkers.td,clang-sa-checkers))
endif

ifneq ($(filter CommentCommandInfo.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentCommandInfo.inc, \
    $(CLANG_ROOT_PATH)/include/clang/AST/CommentCommands.td,clang-comment-command-info))
endif

ifneq ($(filter CommentCommandList.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentCommandList.inc, \
    $(CLANG_ROOT_PATH)/include/clang/AST/CommentCommands.td,clang-comment-command-list))
endif

ifneq ($(filter CommentHTMLNamedCharacterReferences.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentHTMLNamedCharacterReferences.inc, \
    $(CLANG_ROOT_PATH)/include/clang/AST/CommentHTMLNamedCharacterReferences.td,clang-comment-html-named-character-references))
endif

ifneq ($(filter CommentHTMLTagsProperties.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentHTMLTagsProperties.inc, \
    $(CLANG_ROOT_PATH)/include/clang/AST/CommentHTMLTags.td,clang-comment-html-tags-properties))
endif

ifneq ($(filter CommentHTMLTags.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentHTMLTags.inc, \
    $(CLANG_ROOT_PATH)/include/clang/AST/CommentHTMLTags.td,clang-comment-html-tags))
endif

ifneq ($(filter CommentNodes.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentNodes.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/CommentNodes.td,clang-comment-nodes))
endif

$(foreach genfile,$(filter Diagnostic%Kinds.inc,$(TBLGEN_TABLES)), \
    $(eval $(call define-clang-tblgen-rule, \
        $(generated_sources)/include/clang/Basic/$(genfile), \
        $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td,clang-diags-defs \
        -clang-component=$(patsubst Diagnostic%Kinds.inc,%,$(genfile)))))

ifneq ($(filter DiagnosticGroups.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/DiagnosticGroups.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td,clang-diag-groups))
endif

ifneq ($(filter DiagnosticIndexName.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/DiagnosticIndexName.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td,clang-diag-groups))
endif

ifneq ($(filter DeclNodes.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/DeclNodes.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/DeclNodes.td,clang-decl-nodes))
endif

ifneq ($(filter StmtNodes.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/StmtNodes.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/StmtNodes.td,clang-stmt-nodes))
endif

ifneq ($(filter arm_neon.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/arm_neon.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/arm_neon.td,arm-neon-sema))
endif

ifneq ($(filter Options.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-llvm-tblgen-rule, \
    $(generated_sources)/include/clang/Driver/Options.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Driver/Options.td,opt-parser-defs))
endif

ifneq ($(filter arm_neon.h,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/arm_neon.h, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/arm_neon.td,arm-neon))
endif

LOCAL_C_INCLUDES += $(generated_sources)/include
LOCAL_EXPORT_C_INCLUDE_DIRS += $(generated_sources)/include

endif

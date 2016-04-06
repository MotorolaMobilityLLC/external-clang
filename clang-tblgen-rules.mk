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

ifneq ($(findstring AttrDump,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/AttrDump.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-dump))
endif

ifneq ($(findstring AttrImpl,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/AttrImpl.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-impl))
endif

ifneq ($(findstring AttrHasAttributeImpl,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/AttrHasAttributeImpl.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-has-attribute-impl))
endif

ifneq ($(findstring AttrList,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/AttrList.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-list))
endif

ifneq ($(findstring AttrSpellingListIndex,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Sema/AttrSpellingListIndex.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-spelling-index))
endif

ifneq ($(findstring AttrPCHRead,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Serialization/AttrPCHRead.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-pch-read))
endif

ifneq ($(findstring AttrPCHWrite,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Serialization/AttrPCHWrite.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-pch-write))
endif

ifneq ($(findstring Attrs,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/Attrs.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-classes))
endif

ifneq ($(findstring AttrParserStringSwitches,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Parse/AttrParserStringSwitches.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-parser-string-switches))
endif

ifneq ($(findstring AttrVisitor,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/AttrVisitor.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-ast-visitor))
endif

ifneq ($(findstring AttrParsedAttrKinds,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Sema/AttrParsedAttrKinds.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-parsed-attr-kinds))
endif

ifneq ($(findstring AttrParsedAttrImpl,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Sema/AttrParsedAttrImpl.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-parsed-attr-impl))
endif

ifneq ($(findstring AttrParsedAttrList,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Sema/AttrParsedAttrList.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-parsed-attr-list))
endif

ifneq ($(findstring AttrTemplateInstantiate,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Sema/AttrTemplateInstantiate.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td,clang-attr-template-instantiate))
endif

ifneq ($(findstring Checkers.inc,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/Checkers.inc, \
    $(CLANG_ROOT_PATH)/lib/StaticAnalyzer/Checkers/Checkers.td,clang-sa-checkers))
endif

ifneq ($(findstring CommentCommandInfo,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentCommandInfo.inc, \
    $(CLANG_ROOT_PATH)/include/clang/AST/CommentCommands.td,clang-comment-command-info))
endif

ifneq ($(findstring CommentCommandList,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentCommandList.inc, \
    $(CLANG_ROOT_PATH)/include/clang/AST/CommentCommands.td,clang-comment-command-list))
endif

ifneq ($(findstring CommentHTMLNamedCharacterReferences,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentHTMLNamedCharacterReferences.inc, \
    $(CLANG_ROOT_PATH)/include/clang/AST/CommentHTMLNamedCharacterReferences.td,clang-comment-html-named-character-references))
endif

ifneq ($(findstring CommentHTMLTagsProperties,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentHTMLTagsProperties.inc, \
    $(CLANG_ROOT_PATH)/include/clang/AST/CommentHTMLTags.td,clang-comment-html-tags-properties))
endif

ifneq ($(findstring CommentHTMLTags,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentHTMLTags.inc, \
    $(CLANG_ROOT_PATH)/include/clang/AST/CommentHTMLTags.td,clang-comment-html-tags))
endif

ifneq ($(findstring CommentNodes,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/CommentNodes.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/CommentNodes.td,clang-comment-nodes))
endif

$(foreach genfile,$(filter Diagnostic%Kinds.inc,$(TBLGEN_TABLES)), \
    $(eval $(call define-clang-tblgen-rule, \
        $(generated_sources)/include/clang/Basic/$(genfile), \
        $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td,clang-diags-defs \
        -clang-component=$(patsubst Diagnostic%Kinds.inc,%,$(genfile)))))

ifneq ($(findstring DiagnosticGroups,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/DiagnosticGroups.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td,clang-diag-groups))
endif

ifneq ($(findstring DiagnosticIndexName,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/DiagnosticIndexName.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td,clang-diag-groups))
endif

ifneq ($(findstring DeclNodes,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/DeclNodes.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/DeclNodes.td,clang-decl-nodes))
endif

ifneq ($(findstring StmtNodes,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/AST/StmtNodes.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/StmtNodes.td,clang-stmt-nodes))
endif

ifneq ($(findstring arm_neon,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/arm_neon.inc, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/arm_neon.td,arm-neon-sema))
endif

ifneq ($(findstring Options.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Driver/Options.inc
# We cannot use define-tblgen-rule in external/llvm/llvm-tblgen-rules.mk
# as define-tblgen-rule may be not yet. We use transform-td-to-out,
# which is evaluated at recipe-evaluation-time and is always available.
$(generated_sources)/include/clang/Driver/Options.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Driver/Options.inc: $(CLANG_ROOT_PATH)/include/clang/Driver/Options.td $(CLANG_TBLGEN) $(LLVM_TBLGEN)
	$(call transform-td-to-out,opt-parser-defs)
$(call include-depfile, \
    $(generated_sources)/include/clang/Driver/Options.inc.d, \
    $(generated_sources)/include/clang/Driver/Options.inc)
endif

ifneq ($(findstring arm_neon.h,$(TBLGEN_TABLES)),)
$(eval $(call define-clang-tblgen-rule, \
    $(generated_sources)/include/clang/Basic/arm_neon.h, \
    $(CLANG_ROOT_PATH)/include/clang/Basic/arm_neon.td,arm-neon))
endif

LOCAL_C_INCLUDES += $(generated_sources)/include

endif

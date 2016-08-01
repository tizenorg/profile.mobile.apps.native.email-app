BUILD_SCRIPT_VERSION := 1.0.7

all : app_build

clean : app_clean

version : make_version


BSLASH := \\#
BSLASH2SLASH = $(subst $(BSLASH),/,$(1))
REMOVE_TAIL = $(patsubst %/,%,$(1))

PROJ_ROOT := $(call BSLASH2SLASH,$(PROJPATH))

-include $(PROJ_ROOT)/project_def.prop
-include basedef.mk
-include tooldef.mk
-include flags.mk
-include prepost.mk

APPTYPE := $(type)

OUTPUT_DIR := $(call BSLASH2SLASH,$(OUTPUT_DIR))
OBJ_OUTPUT := $(call BSLASH2SLASH,$(OUTPUT_DIR)/objs)

OS_NAME = $(shell $(UNAME))

#LOWER_APPNAME := $(shell echo translit($(APPNAME),[A-Z],[a-z])|$(M4))
LOWER_APPNAME := $(shell echo $(APPNAME)|$(TR) [A-Z] [a-z])

ifeq ($(strip $(APPTYPE)),app)
APPFILE := $(OUTPUT_DIR)/$(LOWER_APPNAME)
endif
ifeq ($(strip $(APPTYPE)),staticLib)
APPFILE := $(OUTPUT_DIR)/lib$(LOWER_APPNAME).a
endif
ifeq ($(strip $(APPTYPE)),sharedLib)
APPFILE := $(OUTPUT_DIR)/lib$(LOWER_APPNAME).so
endif

ifneq ($(strip $(PLATFORM_INCS)),)
PLATFORM_INCS_FILE := $(OBJ_OUTPUT)/platform_incs_file.inc
endif

FIND_FILES = $(shell $(FIND) $(PROJ_ROOT)/$(dir $(1)) -type f -name $(notdir $(1)) -printf '$(dir $(1))%P ')

NORMAL_SRCS := $(filter-out %*.c %*.cpp,$(USER_SRCS))
WIDLCARD_SRCS := $(filter %*.c %*.cpp,$(USER_SRCS))
ALL_SRCS := $(NORMAL_SRCS) $(foreach var,$(WIDLCARD_SRCS),$(call FIND_FILES,$(var)))

C_SRCS := $(filter %.c,$(ALL_SRCS))
CPP_SRCS := $(filter %.cpp,$(ALL_SRCS))

USER_EDCS := $(subst $(BSLASH),/,$(USER_EDCS))
NORMAL_EDCS := $(filter-out %*.edc,$(USER_EDCS))
WIDLCARD_EDCS := $(filter %*.edc,$(USER_EDCS))
EDCS := $(NORMAL_EDCS) $(foreach var,$(WIDLCARD_EDCS),$(call FIND_FILES,$(var)))

NORMAL_POS := $(filter-out %*.po,$(USER_POS))
WIDLCARD_POS := $(filter %*.po,$(USER_POS))
POS := $(NORMAL_POS) $(foreach var,$(WIDLCARD_POS),$(call FIND_FILES,$(var)))

LIBPATHS := $(addprefix -L$(PROJ_ROOT)/,$(USER_LIB_DIRS)) \
			$(addprefix -L,$(USER_LIB_DIRS_ABS))
LIBS += $(addprefix -l,$(USER_LIBS))
UOBJS := $(addprefix $(PROJ_ROOT)/,$(USER_OBJS)) \
		 $(USER_OBJS_ABS)

M_OPT = -MMD -MP -MF"$(@:%.o=%.d)"

FUNC_C2O = $(patsubst %.c,$(OBJ_OUTPUT)/%.o,$(notdir $(1)))
FUNC_CPP2O = $(patsubst %.cpp,$(OBJ_OUTPUT)/%.o,$(notdir $(1)))
FUNC_EDC2EDCD = $(patsubst %.edc,$(OBJ_OUTPUT)/%.edc.d,$(notdir $(1)))
FUNC_EDC2EDJ = $(patsubst %.edc,$(OUTPUT_DIR)/res/edje/%.edj,$(notdir $(1)))
FUNC_PO2MO = $(patsubst %.po,$(OUTPUT_DIR)/res/locale/%/LC_MESSAGES/$(LOWER_APPNAME).mo,$(notdir $(1)))

C_OBJS := $(call FUNC_C2O,$(C_SRCS))
CPP_OBJS := $(call FUNC_CPP2O,$(CPP_SRCS))
OBJS := $(C_OBJS) $(CPP_OBJS)
EDJ_DEPS := $(call FUNC_EDC2EDCD,$(EDCS))
EDJ_FILES := $(call FUNC_EDC2EDJ,$(EDCS))
MO_FILES := $(call FUNC_PO2MO,$(POS))
DEPS := $(OBJS:.o=.d)


ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif

ifneq ($(strip $(EDJ_DEPS)),)
-include $(EDJ_DEPS)
endif

ifeq ($(strip $(APPTYPE)),app)
$(APPFILE) : $(OBJS) $(UOBJS)
	@echo '  Building target: $@'
	@echo '  Invoking: C/C++ Linker'
	@$(MKDIR) $(MKDIR_OP) $(subst $(BSLASH),/,$(@D))
	$(CXX) -o "$(APPFILE)" $(OBJS) $(UOBJS) $(LIBPATHS) -Xlinker --as-needed $(LIBS) $(LINK_FLAGS) $(TC_LINKER_MISC) $(RS_LINKER_MISC) -pie -lpthread -Xlinker -rpath="/home/developer/sdk_tools/lib" --sysroot="$(SYSROOT)" -Xlinker --version-script="$(PROJ_ROOT)/.exportMap" -L"$(SBI_SYSROOT)/usr/lib" $(RS_LIBRARIES) -Xlinker -rpath="/opt/usr/apps/$(APPID)/lib" -Werror-implicit-function-declaration
	@echo '  Finished building target: $@'
endif
ifeq ($(strip $(APPTYPE)),staticLib)
$(APPFILE) : $(OBJS) $(UOBJS)
	@echo '  Building target: $@'
	@echo '  Invoking: Archive utility'
	@$(MKDIR) $(MKDIR_OP) $(subst $(BSLASH),/,$(@D))
	$(AR) -r "$(APPFILE)" $(OBJS) $(UOBJS) $(AR_FLAGS)
	@echo '  Finished building target: $@'
endif
ifeq ($(strip $(APPTYPE)),sharedLib)
$(APPFILE) : $(OBJS) $(UOBJS)
	@echo '  Building target: $@'
	@echo '  Invoking: C/C++ Linker'
	@$(MKDIR) $(MKDIR_OP) $(subst $(BSLASH),/,$(@D))
	$(CXX) -o "$(APPFILE)" $(OBJS) $(UOBJS) $(LIBPATHS) -Xlinker --as-needed $(LIBS) $(LINK_FLAGS) $(TC_LINKER_MISC) $(RS_LINKER_MISC) -shared -lpthread --sysroot="$(SYSROOT)" -L"$(SYSROOT)/usr/lib" $(RS_LIBRARIES)
	@echo '  Finished building target: $@'
endif


ifneq ($(strip $(C_SRCS)),)
CDEFS += $(addprefix -D,$(USER_DEFS))
CDEFS += $(addprefix -U,$(USER_UNDEFS))

INCS := $(addprefix -I$(PROJ_ROOT)/,$(USER_INC_DIRS)) \
		$(addprefix -I,$(USER_INC_DIRS_ABS)) \
		$(addprefix -include$(PROJ_ROOT)/,$(USER_INC_FILES)) \
		$(addprefix -include,$(USER_INC_FILES_ABS))

define C_RULE
$$(call FUNC_C2O,$(1)) : $$(PROJ_ROOT)/$(1) $$(PLATFORM_INCS_FILE)
	@echo '  Building file: $$<'
	@echo '  Invoking: C Compiler'
	@$$(MKDIR) $$(MKDIR_OP) $$(subst $$(BSLASH),/,$$(@D))
ifeq ($$(strip $$(APPTYPE)),sharedLib)
	$$(CC) -c $$< -o $$@ $$(CDEFS) $$(INCS) -I"pch" $$(COMPILE_FLAGS) $$(TC_COMPILER_MISC) $$(RS_COMPILER_MISC) -fPIC --sysroot="$$(SYSROOT)" -Werror-implicit-function-declaration $$(M_OPT) @$$(PLATFORM_INCS_FILE)
else
	$$(CC) -c $$< -o $$@ $$(CDEFS) $$(INCS) -I"pch" $$(COMPILE_FLAGS) $$(TC_COMPILER_MISC) $$(RS_COMPILER_MISC) -fPIE --sysroot="$$(SYSROOT)" -Werror-implicit-function-declaration $$(M_OPT) @$$(PLATFORM_INCS_FILE)
endif
	@echo '  Finished building: $$<'
endef

$(foreach var,$(C_SRCS),$(eval $(call C_RULE,$(var))))
endif

ifneq ($(strip $(CPP_SRCS)),)
CPPDEFS += $(addprefix -D,$(USER_CPP_DEFS))
CPPDEFS += $(addprefix -U,$(USER_CPP_UNDEFS))

CPP_INCS := $(addprefix -I$(PROJ_ROOT)/,$(USER_CPP_INC_DIRS)) \
		$(addprefix -I,$(USER_CPP_INC_DIRS_ABS)) \
		$(addprefix -include$(PROJ_ROOT)/,$(USER_CPP_INC_FILES)) \
		$(addprefix -include,$(USER_CPP_INC_FILES_ABS))

define CPP_RULE
$$(call FUNC_CPP2O,$(1)) : $$(PROJ_ROOT)/$(1) $$(PLATFORM_INCS_FILE)
	@echo '  Building file: $$<'
	@echo '  Invoking: C++ Compiler'
	@$$(MKDIR) $$(MKDIR_OP) $$(subst $$(BSLASH),/,$$(@D))
ifeq ($$(strip $$(APPTYPE)),sharedLib)
	$$(CXX) -c $$< -o $$@ $$(CPPDEFS) $$(CPP_INCS) -I"pch" $$(CPP_COMPILE_FLAGS) $$(TC_COMPILER_MISC) $$(RS_COMPILER_MISC) -fPIC --sysroot="$$(SYSROOT)" -Werror-implicit-function-declaration $$(M_OPT) @$$(PLATFORM_INCS_FILE)
else
	$$(CXX) -c $$< -o $$@ $$(CPPDEFS) $$(CPP_INCS) -I"pch" $$(CPP_COMPILE_FLAGS) $$(TC_COMPILER_MISC) $$(RS_COMPILER_MISC) -fPIE --sysroot="$$(SYSROOT)" -Werror-implicit-function-declaration $$(M_OPT) @$$(PLATFORM_INCS_FILE)
endif
	@echo '  Finished building: $$<'
endef

$(foreach var,$(CPP_SRCS),$(eval $(call CPP_RULE,$(var))))
endif


preproc :
ifneq ($(strip $(PREBUILD_COMMAND)),)
ifneq ($(strip $(PREBUILD_DESC)),)
	@echo $(PREBUILD_DESC)
endif
	@echo $(shell $(PREBUILD_COMMAND))
endif


postproc :
ifneq ($(strip $(POSTBUILD_COMMAND)),)
ifneq ($(strip $(POSTBUILD_DESC)),)
	@echo $(POSTBUILD_DESC)
endif
	@echo $(shell $(POSTBUILD_COMMAND))
endif

ifneq ($(strip $(PLATFORM_INCS)),)
$(PLATFORM_INCS_FILE) :
	@echo '  Building inc file: $@'
	@$(MKDIR) $(MKDIR_OP) $(OBJ_OUTPUT)
	@echo $(PLATFORM_INCS) > $@
endif

define EDJ_PROC
EDC := $$($(2))

ifneq ($$(strip $$(EDC)),)
EDJ_FILE := $$(call FUNC_EDC2EDJ,$$(EDC))

EDC_$(1)_COMPILER_FLAGS := -id "$$(SDK_TOOLPATH)/enventor/share/enventor/images"
EDC_$(1)_COMPILER_FLAGS += -sd "$$(SDK_TOOLPATH)/enventor/share/enventor/sounds"
EDC_$(1)_COMPILER_FLAGS += -fd "$$(SDK_TOOLPATH)/enventor/share/enventor/fonts"

ifneq ($$(strip $(3)),)
EDC_$(1)_COMPILER_FLAGS += $$(addprefix -id $$(PROJ_ROOT)/,$$($(3)))
endif
ifneq ($$(strip $(4)),)
EDC_$(1)_COMPILER_FLAGS += $$(addprefix -id ,$$($(4)))
endif
ifneq ($$(strip $(5)),)
EDC_$(1)_COMPILER_FLAGS += $$(addprefix -sd $$(PROJ_ROOT)/,$$($(5)))
endif
ifneq ($$(strip $(6)),)
EDC_$(1)_COMPILER_FLAGS += $$(addprefix -sd ,$$($(6)))
endif
ifneq ($$(strip $(7)),)
EDC_$(1)_COMPILER_FLAGS += $$(addprefix -fd $$(PROJ_ROOT)/,$$($(7)))
endif
ifneq ($$(strip $(8)),)
EDC_$(1)_COMPILER_FLAGS += $$(addprefix -fd ,$$($(8)))
endif

$$(EDJ_FILE) : $$(PROJ_ROOT)/$$(EDC)
	@echo '  Building file: $$<'
	@echo '  Invoking: EDC Resource Compiler'
	@$$(MKDIR) $$(MKDIR_OP) $$(subst $$(BSLASH),/,$$(@D))
	$(CC) -x c -M -MP "$$<" -MT "$$@" -MF $$(call FUNC_EDC2EDCD,$$<)
	$$(EDJE_CC) $$(EDC_$(1)_COMPILER_FLAGS) $$(CDEFS) "$$<" "$$@"
	@echo '  Finished building: $$<'
endif
endef

# Global EDCs
ifneq ($(strip $(EDCS)),)
$(foreach var_edc,$(EDCS),$(eval $(call EDJ_PROC,,var_edc,USER_EDCS_IMAGE_DIRS,USER_EDCS_IMAGE_DIRS_ABS,USER_EDCS_SOUND_DIRS,USER_EDCS_SOUND_DIRS_ABS,USER_EDCS_FONT_DIRS,USER_EDCS_FONT_DIRS_ABS)))
endif

ifneq ($(strip $(POS)),)
define MO_RULE
$(call FUNC_PO2MO,$(1)) : $(PROJ_ROOT)/$(1)
	@echo '  Building file: $$<'
	@echo '  Invoking: msgfmt String Formatter'
	@$$(MKDIR) $$(MKDIR_OP) $$(subst $$(BSLASH),/,$$(@D))
	$$(MSGFMT) -o $$@ $$<
	@echo '  Finished building: $$<'
endef

$(foreach var,$(POS),$(eval $(call MO_RULE,$(var))))
endif


secondary-outputs : $(EDJ_FILES) $(MO_FILES)

-include appendix.mk


app_build : preproc $(APPFILE) secondary-outputs postproc

app_clean :
	rm -f "$(APPFILE)"
	rm -rf "$(OUTPUT_DIR)"

make_version :
	@echo $(BUILD_SCRIPT_VERSION)

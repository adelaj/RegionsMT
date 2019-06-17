ifeq ($(strip $(ARCH)),)
ARCH = $(shell arch)
endif

ifeq ($(strip $(CFG)),)
CFG = Release
endif

ifeq ($(strip $(LIBRARY_PATH)),)
LIBRARY_PATH = ..
endif

ifeq ($(strip $(INSTALL_PATH)),)
INSTALL_PATH = ..
endif

CC = gcc
CC_OPT = -std=c11 -flto -fuse-linker-plugin -Wall -mavx
CC_OPT-i386 = -m32
CC_OPT-x86_64 = -m64
CC_OPT--Release = -O3
CC_OPT--Debug = -D_DEBUG -Og -ggdb
CC_INC = gsl

LD = $(CC)
LD_OPT = -flto -fuse-linker-plugin -mavx
LD_OPT-i386 = -m32
LD_OPT-x86_64 = -m64
LD_OPT--Release = -O3
LD_OPT--Debug = -Og
LD_LIB = m pthread gsl gslcblas
LD_INC = gsl

TARGET = RegionsMT
OBJ_DIR = obj
SRC_DIR = src

target = $(foreach cfg,$(CFG),$(addsuffix -$(cfg),$(addprefix $1-,$(ARCH))))

.PHONY: all
all: $(call target,build);

.PHONY: clean
clean: | $(call target,clean) $(addprefix clean-,$(ARCH));

rwildcard = $(sort $(wildcard $(addprefix $1,$2))) $(foreach d,$(sort $(wildcard $1*)),$(call rwildcard,$d/,$2))
rev = $(if $1,$(call rev,$(wordlist 2,$(words $1),$1)) $(firstword $1))
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))
tree = $(if $1,$(call tree,$(filter-out ./,$(dir $(patsubst %/,%,$1)))) $(dir $1))

define clean_obj =
.PHONY: $(addprefix clean-,$1)
$(addprefix clean-,$1):; $$(if $$(wildcard $$(patsubst clean-%,%,$$@)),rm $$(patsubst clean-%,%,$$@))
endef

define make_dir =
$1: | $2; mkdir $$@
.PHONY: $(addprefix clean-,$1)
$(addprefix clean-,$1):; $$(if $$(wildcard $$(patsubst clean-%,%,$$@)),$$(if $$(wildcard $$(patsubst clean-%,%,$$@)/*),,rmdir $$(patsubst clean-%,%,$$@)))
endef

define build_dir_arch =
$(eval BUILD_DIR-$1 = $(INSTALL_PATH)/$(TARGET)-$1)
$(call make_dir,$(BUILD_DIR-$1))
.PHONY: clean-$1
clean-$1: clean-$(BUILD_DIR-$1);
endef

define build_dir_arch_cfg =
$(eval BUILD_DIR-$1-$2 = $(BUILD_DIR-$1)/$2)
$(call make_dir,$(BUILD_DIR-$1-$2),$(BUILD_DIR-$1))
endef

define build_var_arch_cfg =
BUILD_$1-$2-$3 = $(strip $(addsuffix $4,$($1)) $(addsuffix $4,$($1-$2)) $(addsuffix $4,$($1--$3)) $(addsuffix $4,$($1-$2-$3)))
endef

define build =
$(eval OBJ_TMP$1 = $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%.o,$(call rwildcard,$(SRC_DIR)/,*.c)))
$(eval STRUCT$1 = $(addprefix $(BUILD_DIR$1)/,$(call uniq,$(foreach m,$(OBJ_TMP$1),$(call tree,$m)))))
$(eval OBJ$1 = $(addprefix $(BUILD_DIR$1)/,$(OBJ_TMP$1)))
$(eval TARGET$1 = $(BUILD_DIR$1)/$(TARGET))
.PHONY: build$1
build$1: $(TARGET$1);
$(call make_dir,$(STRUCT$1),$(BUILD_DIR$1))
$(TARGET$1): $(OBJ$1) | $(BUILD_DIR$1); $(LD) $(BUILD_LD_OPT$1) -o $$@ $$^ $(addprefix -L,$(addprefix $(LIBRARY_PATH)/,$(BUILD_LD_INC$1))) $(addprefix -l,$(BUILD_LD_LIB$1))
$(eval CLEAN-$(BUILD_DIR$1) = $(TARGET$1))
$(OBJ$1): $(BUILD_DIR$1)/$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c | $$(dir $(BUILD_DIR$1)/$(OBJ_DIR)/%) $$(eval CLEAN-$$(dir $(BUILD_DIR$1)/$(OBJ_DIR)/%) += $$@); $(CC) $(BUILD_CC_OPT$1) $(addprefix -I,$(addprefix $(LIBRARY_PATH)/,$(BUILD_CC_INC$1))) -o $$@ -c $$^
#$(call clean_obj,$(TARGET$1) $(OBJ$1))
#.PHONY: clean$1
#clean$1: | $(addprefix clean-,$(TARGET$1) $(call rev,$(OBJ$1)) $(call rev,$(STRUCT$1)) $(BUILD_DIR$1));
endef

$(foreach arch,$(ARCH),\
$(eval $(call build_dir_arch,$(arch)))\
$(foreach cfg,$(CFG),\
$(eval $(call build_dir_arch_cfg,$(arch),$(cfg)))\
$(foreach var,CC_OPT LD_OPT LD_LIB,$(eval $(call build_var_arch_cfg,$(var),$(arch),$(cfg),)))\
$(foreach var,CC_INC LD_INC,$(eval $(call build_var_arch_cfg,$(var),$(arch),$(cfg),-$(arch)/$(cfg))))\
$(eval $(call build,-$(arch)-$(cfg)))))

CLEAN-$(BUILD_DIR-x86_64-Release) = $(TARGET-x86_64-Release)

print-CLEAN-../RegionsMT-x86_64/Release:; @echo print-CLEAN-../RegionsMT-x86_64/Release = $(print-CLEAN-../RegionsMT-x86_64/Release)

.PHONY: print-%
print-%:; @echo $* = $($*)

.DEFAULT_GOAL = all

VALID_LIBRARY := gsl
VALID_ARCH := x86_64 i386 i686
VALID_CONFIG := Debug Release
VALID_TOOLCHAIN := Xcode msvc gcc gcc-% clang clang-% icc

include common.mk
include env.mk
include var.mk
include gather.mk

LIBRARY ?= $(VALID_LIBRARY)
LIBRARY := $(call uniq,$(filter $(VALID_LIBRARY),$(LIBRARY)))

TARGET := $(LIBRARY)

define build_cmake =



endef

URL-gsl := https://github.com/ampl/gsl

GATHER_CLEAN_PROJ :=
GATHER_CLEAN_SRC :=
GATHER_CLEAN_LOG :=
DOWNLOAD_GIT := $(addprefix download-,$(TARGET))

.PHONY: $(DOWNLOAD_GIT)
$(DOWNLOAD_GIT): download-%:
    $(if $(wildcard $(BUILD_PATH)/$*),\
    git -C "$(BUILD_PATH)/$*" reset --hard HEAD > "$(BUILD_PATH)/$*.log" && \
    git -C "$(BUILD_PATH)/$*" pull >> "$(BUILD_PATH)/$*.log",\
    git clone --depth 1 "$(URL-$*)" "$(BUILD_PATH)/$*" > "$(BUILD_PATH)/$*.log")

$(call gather,LOG,$(BUILD_PATH)/gsl.log)
$(call gather,SRC,$(BUILD_PATH)/gsl)

GATHER_CLEAN_FILE += GATHER_CLEAN_LOG
$(call gather_clean_file)

.PHONY: $(GATHER_CLEAN_PROJ)
.SECONDEXPANSION:
$(GATHER_CLEAN_PROJ): clean-%: | $$(CLEAN-%)
    $(if $(wildcard $*),rm -rf $*)

.PHONY: all
all: $(call foreachl,2 3 4 5,id,$$($$2/$$3/$$4/$$5),$(TARGET),$(TOOLCHAIN),$(ARCH),$(CFG));

.PHONY: clean
clean: | $(GATHER_CLEAN_LOG) $(call foreachl,2,id,clean-$$(DIR/$$2),$(TOOLCHAIN)) $(call foreachl,2 3 4,id,clean-$$(DIR-$$2/$$3/$$4),$(TARGET),$(TOOLCHAIN),$(ARCH));

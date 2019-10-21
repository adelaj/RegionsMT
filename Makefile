.DEFAULT_GOAL = all

include mk/common.mk
include mk/env.mk
include mk/var.mk
include mk/gather.mk
include mk/build.mk
include mk/contrib.mk

TARGET := RegionsMT

$(call var,LDFLAGS,$(TARGET),$(CC_TOOLCHAIN),%,%,$(addprefix -l,m pthread))
$(call var,CREQ,$(TARGET),$(CC_TOOLCHAIN),%,%,$$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/copy-headers.log)
#$(call var,LDREQ,$(TARGET),$(CC_TOOLCHAIN),%,%,$$$$(addprefix $$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/,libgsl.a libgslcblas.a))

$(call cc,$(TARGET),$(TOOLCHAIN),$(ARCH),$(CFG))

$(call foreachl,1,var_base,GATHER_DIR GATHER_DIST GATHER_CONTRIB GATHER_FILE GATHER_INC,,+=)

$(call var_base,CLEAN,GATHER_FILE)

.PHONY: all
all: $(GATHER_FILE);

.PHONY: clean
clean: | $(call decorate,clean,$(GATHER_FILE) $(GATHER_DIR));

include $(wildcard $(GATHER_INC))

$(call gather_dir,$(GATHER_DIR))
$(call clean_dir_opaq,$(GATHER_DIR_OPAQ))
$(call clean_dir,$(GATHER_DIR))
$(call clean_file,$(GATHER_FILE))

.SECONDEXPANSION:
$(call prereq,$(GATHER_FILE) $(GATHER_DIR) $(GATHER_DIR_OPAQ))

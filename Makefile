.DEFAULT_GOAL = all

include mk/common.mk
include mk/env.mk
include mk/var.mk
include mk/gather.mk
include mk/build.mk
include mk/contrib.mk

TARGET := RegionsMT

$(call var,LDFLAGS,$(TARGET),$(CC_TOOLCHAIN),%,%,$(addprefix -l,m pthread))
$(call var,CREQ,$(TARGET),%,%,%,copy-headers($$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5))
$(call var,LDREQ,$(TARGET),%,%,%,gsl($$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5) gslcblas($$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5))

$(call cc,$(TARGET),$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG))

$(call foreachl,1,var_base,GATHER_DIR GATHER_DIST GATHER_FILE GATHER_INC GATHER_DIST_CLEAN GATHER_CLEAN,,.)

.PHONY: all
all: $(GATHER_FILE);

.PHONY: distclean
distclean: | $(GATHER_DIST_CLEAN);

.PHONY: clean
clean: | $(GATHER_CLEAN);

include $(wildcard $(GATHER_INC))

$(call gather_mkdir,$(GATHER_DIR))
$(call gather_rm_r,$(GATHER_DIST),GATHER_DIST_CLEAN)
$(call gather_rmdir,$(GATHER_DIR),GATHER_CLEAN)
$(call gather_rm,$(GATHER_FILE),GATHER_CLEAN)
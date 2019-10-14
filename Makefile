.DEFAULT_GOAL = all

include mk/common.mk
include mk/env.mk
include mk/var.mk
include mk/gather.mk
include mk/build.mk
include mk/contrib.mk

TARGET := RegionsMT

$(call var,LDFLAGS,$(TARGET),$(BUILD_CC_TOOLCHAIN),%,%,$(addprefix -l,m pthread))
$(call var,CSRC,$(TARGET),%,%,%,$$(call rwildcard,$$(PREFIX)/$$2/src/,*.c))
$(call var,CINC,$(TARGET),%,%,%,$$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5)
$(call var,LDREQ,$(TARGET),$(BUILD_CC_TOOLCHAIN),%,%,$$$$(adprefix $$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/,libgsl.a libgslcblas.a))

$(call build_cc,$(call var_decl,$(TARGET),$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG),$$(PREFIX)/$$2/$$1/$$3/$$4/$$1))

$(call foreachl,1,var_base,GATHER_DIR GATHER_DIST GATHER_FILE GATHER_INC,,.)

$(call gather_mkdir,$(GATHER_DIR))
$(call gather_rm_r,$(GATHER_DIST:%=clean$(LP)%$(RP)))
$(call gather_rmdir,$(GATHER_DIR:%=clean$(LP)%$(RP)))
$(call gather_rm,$(GATHER_FILE:%=clean$(LP)%$(RP)))

.PHONY: all
all: $(GATHER_FILE);

.PHONY: distclean
distclean: | $(GATHER_DIST:%=clean$(LP)%$(RP));

.PHONY: clean
clean: | $(GATHER_DIR:%=clean$(LP)%$(RP)) $(GATHER_FILE:%=clean$(LP)%$(RP));

include $(wildcard $(GATHER_INC))


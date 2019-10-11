.DEFAULT_GOAL = all

include mk/common.mk
include mk/env.mk
include mk/var.mk
include mk/gather.mk
include mk/build.mk

TARGET := RegionsMT

$(call var,LDFLAGS,$(TARGET),$(BUILD_CC_TOOLCHAIN),%,%,$(addprefix -l,m pthread))
$(call var,CSRC,$(TARGET),%,%,%,$$(call rwildcard,$$(PREFIX)/$$2/src/,*.c))
$(call var,CINC,$(TARGET),%,%,%,$$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5)
$(call var,LDREQ,$(TARGET),$(BUILD_CC_TOOLCHAIN),%,%,$$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/libgsl.a $$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/libgslcblas.a)

$(call build_cc,$(call var_decl,$(TARGET),$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG),$$(PREFIX)/$$2/$$1/$$3/$$4/$$1))

$(call gather_dir)
$(call gather_clean_dist_dir)
$(call gather_clean_dir)
$(call gather_clean_file)

$(call var_base,GATHER_FILE,,0)
$(call var_base,GATHER_INC,,0)

.PHONY: all
all: $(GATHER_FILE);

.PHONY: distclean
distclean: | $(GATHER_CLEAN_DIST_DIR);

.PHONY: clean
clean: | $(GATHER_CLEAN_DIR) $(GATHER_CLEAN_FILE);

include $(wildcard $(GATHER_INC))


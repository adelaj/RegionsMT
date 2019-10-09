.DEFAULT_GOAL = all

include mk/common.mk
include mk/env.mk
include mk/var.mk
include mk/gather.mk
include mk/build.mk

TARGET := RegionsMT

$(call var,CFLAGS,$(TARGET),%,%,%,-I$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5)
$(call var,LDFLAGS,$(TARGET),gcc gcc-% clang clang-% icc,%,%,$(addprefix -l,m pthread))
$(call var,LDEXT,$(TARGET),gcc gcc-% clang clang-% icc,%,%,$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/libgsl.a $(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/libgslcblas.a)

ALL := $(call var_vect,$(TARGET),$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG),$(PREFIX)/$$2/$$1/$$3/$$4)

.PHONY: all
all: $(foreach i,$(ALL),$($i));

$(call gather_dir)
$(call gather_clean_dist_dir)
$(call gather_clean_dir)
$(call gather_clean_file)

.PHONY: distclean
distclean: | $(GATHER_CLEAN_DIST_DIR);

.PHONY: clean
clean: | $(GATHER_CLEAN_DIR) $(GATHER_CLEAN_FILE);
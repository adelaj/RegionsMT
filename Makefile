.DEFAULT_GOAL = all

include mk/common.mk
include mk/env.mk
include mk/var.mk
include mk/gather.mk
include mk/build.mk

TARGET := RegionsMT
$(call var,REQUIRE,$(TARGET),%,%,%,gsl gslcblas)

$(call var,LDFLAGS,$(TARGET),%,%,%,$(addprefix -l,m pthread))
$(call var,REQUIRE,$(TARGET),gcc gcc-% clang clang-%,%,%,gsl:libgsl.a gsl:libgslcblas.a)

ALL := $(call var_vect,$(TARGET),$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG),$(PREFIX)/$$2/$$1/$$3/$$4)

.PHONY: all
all: $(foreach i,$(ALL),$($i));

$(call gather_dir)
$(call gather_clean_dir)
$(call gather_clean_file)

.PHONY: clean
clean: | $(call foreachl,2,id,clean-$$(DIR/$$2),$(TOOLCHAIN)) $(call foreachl,2 3 4,id,clean-$$(DIR-$$2/$$3/$$4),$(TARGET),$(TOOLCHAIN),$(ARCH));
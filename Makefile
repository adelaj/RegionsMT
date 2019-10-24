.DEFAULT_GOAL = all

include mk/common.mk
include mk/env.mk
include mk/var.mk
include mk/gather.mk
include mk/build.mk
include mk/contrib.mk

TARGET := RegionsMT

$(call var,$(addprefix -l,m pthread),$$1,LDFLAGS,$(TARGET),$(CC_TOOLCHAIN),%:%)
$(call var,$$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/copy-headers.log,$$1,CREQ,$(TARGET),$(CC_TOOLCHAIN),%:%)
$(call var,$$$$(addprefix $$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/,libgsl.a libgslcblas.a),$$1,LDREQ,$(TARGET),$(CC_TOOLCHAIN),%:%)

$(call var,/W4,$$1,CFLAGS,$(TARGET),msvc:%:%)

.PHONY: all
all: $(call cc,$(TARGET),$(TOOLCHAIN),$(ARCH),$(CFG))

include $(wildcard $(call safe_var,INCLUDE))

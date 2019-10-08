include mk/common.mk
include mk/var.mk

ARCH ?= $(shell arch)
CFG ?= Release
TOOLCHAIN ?= gcc
PREFIX ?= ..

override ARCH := $(call uniq, $(ARCH))
override CFG := $(call uniq,$(CFG))
override TOOLCHAIN := $(foreach i,$(TOOLCHAIN),$(call rremsuffix,:,$(call compress,,$(wordlist 1,3,$(subst :, : ,$i)))))
override TOOLCHAIN := $(foreach i,$(call uniq,$(call firstsep,:,$(TOOLCHAIN))),$(patsubst %:,%,$(lastword $(filter $i:%,$(addsuffix :,$(TOOLCHAIN))))))
override PREFIX := $(lastword $(PREFIX))

$(call var,gsl,$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG),$(PREFIX)/$$2/$$1/$$3/$$4)

TARGET := RegionsMT

#$(call var,REQUIRE,$1,$2,$3,$4,$(foreach i,$5,$$($$(addsuffix :$$2:$$3:$$4,$(firstword $(subst :, ,$i))))/$(word 2,$(subst :, ,$i))))
#$(call var,CFLAGS,$1,$2,$3,$4,$$(addprefix -L,$$($$(addsuffix :$$2:$$3:$$4,$(call firstsep,:,$5)))))
require =\
$(call var,REQUIRE,$1,$2,$3,$4,$(foreach i,$5,$$($$(addsuffix :$$2:$$3:$$4,$(firstword $(subst :, ,$i))))/$(word 2,$(subst :, ,$i))))\
$(call var,CFLAGS,$1,$2,$3,$4,$(call uniq,$(foreach i,$5,-I$$($(firstword $(subst :, ,$i)):$$3:$$4:$$5))))\
$(call var,LDFLAGS,$1,$2,$3,$4,$(call uniq,$(foreach i,$5,-L$$($(firstword $(subst :, ,$i)):$$3:$$4:$$5) $(word 2,$(subst :, ,$i)))))

$(call var,REQUIRE,$(TARGET),gcc gcc-% clang clang-%,%,%,$(PREFIX)/$$$$2/$$$$1/$$$$3/$$$$4)

#$(call require,$(TARGET),$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG),gsl:libgsl.a gsl:libgslcblas.a)

apply_var = 

abc := xyz 
u := $(call var_vect,a,b,c,$$(strip $$1   $$2   $$3))

$(call var,a,b,c,$$(addprefix $$3,$$(addprefix $$2,$$(addprefix $$1,a b c))))

k := $(call find_var,CFLAGS RegionsMT gcc x86_64 Release,$(.VARIABLES))

$(call var,LDFLAGS,RegionsMT,%,%,%,$(addprefix -l,m pthread))
$(call var,CFLAGS,RegionsMT,%,%,%,)

#m := $$(strip a  b  c)
#$(eval $(if 0,a$$(COL)b$$(COL)c := $m))

$(warning $(gsl:gcc:x86_64:Release))
$(warning $(LDFLAGS:%:clang-%:%:Release))
$(warning $(LDFLAGS:RegionsMT:%:%:%))
$(warning $k)
$(warning $(foreach i,$k,$($i)))
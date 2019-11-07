.DEFAULT_GOAL = all

include mk/common.mk
include mk/env.mk
include mk/var.mk
include mk/gather.mk
include mk/build.mk
include mk/contrib.mk

define msvc_cmake_target =
$(addprefix $(P2134)/,$1.exe ALL_BUILD.log RUN_TESTS.log)
$(call gather,$(P2134)/$1.exe,)
$(eval
$(EP2134)/%.exe $(EP2134)/%.log: $(EP213).log $(call fetch_var2,CREQ $(R123),. $1 $2 $3) $(call fetch_var2,LDREQ $(R1234),. $1 $2 $3 $4)
    $(msvc_cmake_build)
$(EP2134)/ALL_BUILD.log: $(EP2134)/$1.exe
$(EP2134)/RUN_TESTS.log: $(EP2134)/ALL_BUILD.log
)
endef

TARGET := RegionsMT

$(call var_reg,$(addprefix -l,m pthread),$$1,LDLIB,$(TARGET),$(CC_TOOLCHAIN),%:%)
$(call var_reg,$$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5.log,$$1,CREQ,$(TARGET),$(CC_TOOLCHAIN),%:%)
$(call var_reg,$$$$(addprefix $$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/,libgsl.a libgslcblas.a),$$1,LDREQ,$(TARGET),$(CC_TOOLCHAIN),%:%)

# Warning! No <CONFIG> in CREQ:<TARGET>:<TOOLCHAIN>:<ARCH>
$(call var_reg,$(patsubst %,$$$$(PREFIX)/$$$$3/%/$$$$4.log,gsl pthread-win32),$$1,CREQ,$(TARGET),$(MSVC_TOOLCHAIN),%)
$(call var_reg,$(addprefix $$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/,gsl.lib gslcblas.lib) $$$$(PREFIX)/$$$$3/pthread-win32/$$$$4/$$$$5/pthreadVSE2.lib,$$1,LDREQ,$(TARGET),$(MSVC_TOOLCHAIN),%:%)

# Remember to add /D_DLL if we are using dynamic instance of 'pthread-win32' 
$(call var_reg,/W4 /DFORCE_POSIX_THREADS /DPTW32_STATIC_LIB,$$1,CFLAGS,$(TARGET),msvc:%:%)

.PHONY: all $(patsubst %,cmake(%),$(TARGET))
$(patsubst %,cmake(%),$(TARGET)): $(call build,msvc_cmake,$(TARGET),$(call matrix_trunc,1 2,$(MSVC_MATRIX)))
all: $(call build,cc,$(TARGET),$(CC_MATRIX)) $(call build,msvc_cmake_target,$(TARGET),$(MSVC_MATRIX))

.PHONY: clean
clean: | $(do_clean)

include $(wildcard $(call coalesce,INCLUDE,))
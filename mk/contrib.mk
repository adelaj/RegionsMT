define cmake_gsl_base =
$(eval
.PHONY: copy-headers($($1))
copy-headers($($1)): | $($1)/gsl;

$($1)/gsl: | $($1)
    cmake --build "$$%" --target "$$@" -- -j -O VERBOSE=1 COLOR=""

gslcblas($($1)) := $($1)/libgslcblas.a
$($1)/libgslcblas.a: | $($1) copy-headers($($1))
    cmake --build "$$%" --target "$$@" -- -j -O VERBOSE=1 COLOR=""

gsl($($1)) := $($1)/libgsl.a 
$($1)/libgsl.a: $($1)/libgslcblas.a | $($1) copy-headers($($1))
    cmake --build "$$%" --target "$$@" -- -j -O VERBOSE=1 COLOR="")
endef

cmake_gsl = $(call foreachl,1,cmake_gsl_base,$1)

$(call var_base,URL,gsl,git://github.com/ampl/gsl.git,.)
$(call git,$(call var_base_decl,gsl,$$(PREFIX)/$$1,.))

GSL := $(call var_decl,gsl,$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG),$$(PREFIX)/$$2/$$1/$$3/$$4)

$(call cmake,$(GSL))
$(call cmake_gsl,$(GSL))

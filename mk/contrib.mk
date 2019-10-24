cmake_gsl = $(strip $(call vect,1 2 3 4,$1,$2,$3,$4,cmake_gsl_base))
define cmake_gsl_base =
$(if $(and $(filter $(CC_TOOLCHAIN),$(TOOLCHAIN:$2),$(filter $(CC_ARCH),$3))),$(eval
$$(PREFIX)/$$2/$$1/$$3/$$4/lib%.a: $$(PREFIX)/$$2/$$1/$$3/$$4/%.log

$$(PREFIX)/$$2/$$1/$$3/$$4/%.log: $$(PREFIX)/$$2/$$1/$$3/$$4.log
    cmake --build $$(<:%.log=%) --target $$(@F:%.log=%) -- -j -O VERBOSE=1 COLOR="" | tee $$@; \
    $(PIPE_TEST)
),
$(if $(and $(filter $(MSVC_TOOLCHAIN),$(TOOLCHAIN:$2)),$(filter $(MSVC_ARCH),$3)),$(eval
$$(PREFIX)/$$2/$$1/$$3/$$4/%.lib: $$(PREFIX)/$$2/$$1/$$3/$$4/%.log

$$(PREFIX)/$$2/$$1/$$3/$$4/%.log: $$(PREFIX)/$$2/$$1/$$3.log
    powershell "cmake --build $$(<:%.log=%) --target $$(@F:%.log=%) --config $$(notdir $$(@D)) --verbose --parallel -- $(call fetch_var,MSBUILDFLAGS $1 $(TOOLCHAIN:$2) $3 $4)" | tee $$@; \
    $(PIPE_TEST)
)))
$(eval
$(call vect,1,$(PREFIX)/$2/$1/$3/$4/copy-headers.log $(PREFIX)/$2/$1/$3/$4/gslcblas.log $(PREFIX)/$2/$1/$3/$4/gsl.log,,gather)
$$(PREFIX)/$$2/$$1/$$3/$$4/gslcblas.log: $$(PREFIX)/$$2/$$1/$$3/$$4/copy-headers.log
$$(PREFIX)/$$2/$$1/$$3/$$4/gsl.log: $$(PREFIX)/$$2/$$1/$$3/$$4/gslcblas.log)
$(PREFIX)/$2/$1/$3/$4/gsl.log
endef

$(call var_base,git://github.com/ampl/gsl.git,$$1,URL:gsl)

$(call var_base,-D GSL_INSTALL_MULTI_CONFIG=\"ON\",$$1,CMAKEFLAGS:gsl:msvc:%)

.PHONY: git(gsl) cmake_cc(gsl) cmake_msvc(gsl) gsl
git(gsl): $(call git,gsl)
cmake_cc(gsl): $(call cmake_cc,gsl,$(TOOLCHAIN),$(ARCH),$(CFG))
cmake_msvc(gsl): $(call cmake_msvc,gsl,$(TOOLCHAIN),$(ARCH))
gsl: $(call cmake_gsl,gsl,$(TOOLCHAIN),$(ARCH),$(CFG))

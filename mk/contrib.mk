define cmake_gsl_base =
$(if $(filter $(CC_TOOLCHAIN),$(TOOLCHAIN:$2)),$(eval
$$(PREFIX)/$$2/$$1/$$3/$$4/libgslcblas.a $$(PREFIX)/$$2/$$1/$$3/$$4/libgsl.a: $$(PREFIX)/$$2/$$1/$$3/$$4/lib%.a: $$(PREFIX)/$$2/$$1/$$3/$$4/%.log

$$(PREFIX)/$$2/$$1/$$3/$$4/copy-headers.log $$(PREFIX)/$$2/$$1/$$3/$$4/gslcblas.log $$(PREFIX)/$$2/$$1/$$3/$$4/gsl.log: $$(PREFIX)/$$2/$$1/$$3/$$4.log
    cmake --build $$(<:%.log=%) --target $$(@F:%.log=%) -- -j -O VERBOSE=1 COLOR="" | tee $$@; \
    $(PIPE_TEST)    

$$(PREFIX)/$$2/$$1/$$3/$$4/gslcblas.log: $$(PREFIX)/$$2/$$1/$$3/$$4/copy-headers.log

$$(PREFIX)/$$2/$$1/$$3/$$4/gsl.log: $$(PREFIX)/$$2/$$1/$$3/$$4/gslcblas.log))
endef

cmake_gsl = $(call foreachl,1 2 3 4,cmake_gsl_base,$1,$2,$3,$4)

$(call var_base,URL,gsl,git://github.com/ampl/gsl.git,:=)
$(call git,gsl)

$(call cmake,gsl,$(TOOLCHAIN),$(ARCH),$(CFG))
$(call cmake_gsl,gsl,$(TOOLCHAIN),$(ARCH),$(CFG))

cmake_copy = $(strip $(call vect,1,$1,cmake_copy_base))
define cmake_copy_base =
$$(PREFIX)/$1/CMakeLists.txt
$(call gather,$(PREFIX)/$1/CMakeLists.txt,)
$(eval
$$(PREFIX)/$$1/CMakeLists.txt: $$(PREFIX)/$$1.log
    cp contrib/$$(<F:.log=.cmake) $$@)
endef

cmake_cc_build_lib = cmake --build $$(<:.log=) --target $$* -- -j -O VERBOSE=1 COLOR="" | tee $$(@:.a=.log); $(call pipe_test,$$(@:.a=.log))
cmake_msvc_build_lib =  powershell "cmake --build $$(<:.log=) --target $$* --config $$(notdir $$(@D)) --verbose --parallel -- $(call fetch_var,MSBUILDFLAGS $1 $(TOOLCHAIN:$2) $3 $4)" | tee $$(@:.lib=.log); $(call pipe_test,$$(@:.lib=.log))

cmake_gsl = $(strip $(call vect,2 3 4,gsl,$1,$2,$3,cmake_gsl_base))
define cmake_gsl_base =
$(if $(call is_cc,$2,$3,$4),$(PREFIX)/$2/$1/$3/$4/libgsl.a $(PREFIX)/$2/$1/$3/$4/libgslcblas.a$(eval
$$(PREFIX)/$$2/$$1/$$3/$$4/lib%.a: $$(PREFIX)/$$2/$$1/$$3/$$4.log
    $(call cmake_cc_build_lib)
$$(PREFIX)/$$2/$$1/$$3/$$4/libgsl.a: $$(PREFIX)/$$2/$$1/$$3/$$4/libgslcblas.a
),
$(if $(call is_msvc,$2,$3,$4),$(PREFIX)/$2/$1/$3/$4/gslcblas.lib $(PREFIX)/$2/$1/$3/$4/gsl.lib$(eval
$$(PREFIX)/$$2/$$1/$$3/$$4/%.lib: $$(PREFIX)/$$2/$$1/$$3.log
    $(call cmake_msvc_build_lib)
$$(PREFIX)/$$2/$$1/$$3/$$4/gsl.lib: $$(PREFIX)/$$2/$$1/$$3/$$4/gslcblas.lib
)))
endef

$(call var_base,git://github.com/BrianGladman/gsl.git,,URL:gsl)

.PHONY: git(gsl) cmake_copy(gsl) cmake_cc(gsl) cmake_msvc(gsl) gsl
git(gsl): $(call git,gsl)
cmake_copy(gsl): $(call cmake_copy,gsl)
cmake_cc(gsl): $(call cmake_cc,gsl,$(TOOLCHAIN),$(ARCH),$(CFG))
cmake_msvc(gsl): $(call cmake_msvc,gsl,$(TOOLCHAIN),$(ARCH))
gsl: $(call cmake_gsl,$(TOOLCHAIN),$(ARCH),$(CFG))

cmake_copy = $(strip $(call vect,1,$1,cmake_copy_base))
define cmake_copy_base =
$$(PREFIX)/$1/CMakeLists.txt
$(call gather,$(PREFIX)/$1/CMakeLists.txt,)
$(eval
$$(PREFIX)/$$1/CMakeLists.txt: $$(PREFIX)/$$1.log
    cp contrib/$$(<F:.log=.cmake) $$@)
endef

cmake_gsl = $(strip $(call vect,2 3 4,gsl,$1,$2,$3,cmake_gsl_base))
define cmake_gsl_base =
$(if $(call is_cc,$2,$3,$4),$(addprefix $(P2134)/,libgslcblas.a libgsl.a all.log test.log)$(eval
$(EP2134)/lib%.a $(EP2134)/%.log: $(EP2134).log
    cmake --build $$(<:.log=) --target $$* -- -j -O VERBOSE=1 COLOR="" > $$(basename $$@).log || $(call on_error,$$(basename $$@).log)
$(EP2134)/libgsl.a: $(EP2134)/libgslcblas.a
$(EP2134)/all.log: $(EP2134)/libgsl.a
$(EP2134)/test.log: $(EP2134)/all.log
),
$(call gather,$(P2134)/gslcblas.lib,)
$(if $(call is_msvc,$2,$3,$4),$(addprefix $(P2134)/,gslcblas.lib gsl.lib ALL_BUILD.log RUN_TESTS.log)$(eval
$(EP2134)/%.lib $(EP2134)/%.log: $(EP213).log
    powershell "cmake --build $$(<:.log=) --target $$* --config $$(notdir $$(@D)) --verbose --parallel -- \
    $(call fetch_var,MSBUILDFLAGS $1 $(TOOLCHAIN:$2) $3 $4)" > $$(basename $$@).log || $(call on_error,$$(basename $$@).log)
$(EP2134)/gsl.lib: $(EP2134)/gslcblas.lib
$(EP2134)/ALL_BUILD.log: $(EP2134)/gsl.lib
$(EP2134)/RUN_TESTS.log: $(EP2134)/ALL_BUILD.log
)))
endef

$(call var_base,git://github.com/BrianGladman/gsl.git,,URL:gsl)

.PHONY: git(gsl) cmake_copy(gsl) cmake_cc(gsl) cmake_msvc(gsl) gsl
git(gsl): $(call git,gsl)
cmake_copy(gsl): $(call cmake_copy,gsl)
cmake_cc(gsl): $(call cmake_cc,gsl,$(TOOLCHAIN),$(ARCH),$(CFG))
cmake_msvc(gsl): $(call cmake_msvc,gsl,$(TOOLCHAIN),$(ARCH))
gsl: $(call cmake_gsl,$(TOOLCHAIN),$(ARCH),$(CFG))
define cmakelists =
$(strip $$(PREFIX)/$1/CMakeLists.txt
$(call gather,$(PREFIX)/$1/CMakeLists.txt,)
$(eval
$$(PREFIX)/$$1/CMakeLists.txt: $$(PREFIX)/$$1.log
    cp contrib/$$(<F:.log=.cmake) $$@
))
endef

build_gsl = $(call build,$1,gsl,$2)

define cc_cmake_gsl =
$(addprefix $(P2134)/,libgslcblas.a libgsl.a all.log test.log)
$(eval
$(EP2134)/lib%.a $(EP2134)/%.log: $(EP2134).log
    $(strip cmake \
    --build $$(<:.log=) \
    --target $$* \
    -- -j -O VERBOSE=1 COLOR="" $$(if $$(filter test,$$*),ARGS="--output-on-failure") \
    > $$(basename $$@).log \
    || $(call on_error,$$(basename $$@).log))
$(EP2134)/libgsl.a: $(EP2134)/libgslcblas.a
$(EP2134)/all.log: $(EP2134)/libgsl.a
$(EP2134)/test.log: $(EP2134)/all.log
)
endef

define msvc_cmake_gsl =
$(addprefix $(P2134)/,gslcblas.lib gsl.lib ALL_BUILD.log RUN_TESTS.log)
$(call gather,$(P2134)/gslcblas.lib,)
$(eval
$(EP2134)/%.lib $(EP2134)/%.log: $(EP213).log
    powershell "cmake --build $$(<:.log=) --target $$* --config $$(notdir $$(@D)) --verbose --parallel -- \
    $(call fetch_var,MSBUILDFLAGS $1 $(TOOLCHAIN:$2) $3 $4)" > $$(basename $$@).log || $(call on_error,$$(basename $$@).log)
$(EP2134)/gsl.lib: $(EP2134)/gslcblas.lib
$(EP2134)/ALL_BUILD.log: $(EP2134)/gsl.lib
$(EP2134)/RUN_TESTS.log: $(EP2134)/ALL_BUILD.log
)
endef

$(call var_base,git://github.com/BrianGladman/gsl.git,,URL:gsl)

# Fixing bug with 'long double' under MinGW gcc
$(call var_reg,-D__USE_MINGW_ANSI_STDIO,$$1,CFLAGS:gsl,gcc gcc-%,%:%)

.PHONY: git(gsl) cmakelists(gsl) cmake(gsl) cmake(gsl) gsl
git(gsl): $(call git,gsl)
cmakelists(gsl): $(call cmakelists,gsl)
cmake(gsl): $(call build,cc_cmake,gsl,$(CC_MATRIX)) $(call build,msvc_cmake,gsl,$(call matrix_trunc,1 2,$(MSVC_MATRIX)))
gsl: $(call build_gsl,cc_cmake_gsl,$(CC_MATRIX)) $(call build_gsl,msvc_cmake_gsl,$(MSVC_MATRIX))
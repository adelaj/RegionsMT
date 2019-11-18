define cmakelists =
$(strip $$(PREFIX)/$1/CMakeLists.txt
$(call gather,$(PREFIX)/$1/CMakeLists.txt,)
$(eval
$$(PREFIX)/$$1/CMakeLists.txt: $$(PREFIX)/$$1.log
    cp $$(ROOT)/contrib/$$(<F:.log=.cmake) $$@
))
endef

# $(EP2134)/libgsl.a: $(EP2134)/libgslcblas.a
define cc_cmake_gsl =
$(addprefix $(P2134)/,libgslcblas.a libgsl.a all.log test.log)
$(eval
$(EP2134)/lib%.a $(EP2134)/%.log: $(EP2134).log
    $(cc_cmake_build)
$(EP2134)/all.log: $(EP2134)/libgsl.a $(EP2134)/libgslcblas.a
$(EP2134)/test.log: $(EP2134)/all.log
)
endef

# $(EP2134)/gsl.lib: $(EP2134)/gslcblas.lib
define msvc_cmake_gsl =
$(addprefix $(P2134)/,gslcblas.lib gsl.lib ALL_BUILD.log RUN_TESTS.log)
$(call gather,$(addprefix $(P2134)/,gslcblas.lib gsl.lib),)
$(eval
$(EP2134)/%.lib $(EP2134)/%.log: $(EP213).log
    $(msvc_cmake_build)
$(EP2134)/ALL_BUILD.log: $(EP2134)/gsl.lib $(EP2134)/gslcblas.lib
$(EP2134)/RUN_TESTS.log: $(EP2134)/ALL_BUILD.log
)
endef

define msvc_cmake_pthread-win32 =
$(addprefix $(P2134)/,pthreadVSE2.lib pthread-win32.log ALL_BUILD.log RUN_TESTS.log)
$(call gather,$(P2134)/pthreadVSE2.lib,)
$(eval
$(EP2134)/pthreadVSE2.lib: $(EP2134)/pthread-win32.log
$(EP2134)/%.log: $(EP213).log
    $(msvc_cmake_build)
$(EP2134)/ALL_BUILD.log: $(EP2134)/pthreadVSE2.lib
$(EP2134)/RUN_TESTS.log: $(EP2134)/ALL_BUILD.log
)
endef

$(call var_base,git://github.com/BrianGladman/gsl.git,,URL:gsl)
$(call var_base,git://github.com/GerHobbelt/pthread-win32.git,,URL:pthread-win32)

# Fixing bug with 'long double' under MinGW gcc
$(call var_reg,-D__USE_MINGW_ANSI_STDIO,$$1,CFLAGS:gsl,gcc gcc-%,%:%)

.PHONY: git(gsl) cmakelists(gsl) cmake(gsl) test(gsl)
git(gsl): $(call git,gsl)
cmakelists(gsl): $(call cmakelists,gsl)
cmake(gsl): $(call build,cc_cmake,gsl,$(CC_MATRIX)) $(call build,msvc_cmake,gsl,$(call matrix_trunc,1 2,$(MSVC_MATRIX)))
test(gsl): $(call build,cc_cmake_gsl,gsl,$(CC_MATRIX)) $(call build,msvc_cmake_gsl,gsl,$(MSVC_MATRIX))

.PHONY: git(pthread-win32) cmakelists(pthread-win32) cmake(pthread-win32) test(pthread-win32)
git(pthread-win32): $(call git,pthread-win32)
cmakelists(pthread-win32): $(call cmakelists,pthread-win32)
cmake(pthread-win32): $(call build,msvc_cmake,pthread-win32,$(call matrix_trunc,1 2,$(MSVC_MATRIX)))
test(pthread-win32): $(call build,msvc_cmake_pthread-win32,pthread-win32,$(MSVC_MATRIX))
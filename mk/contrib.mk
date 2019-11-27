define cmakelists =
$(eval
$$(PREFIX)/$$1/CMakeLists.txt: $$(ROOT)/contrib/$$1.cmake $$(PREFIX)/$$1.log 
    cp $$< $$@
cmakelists($$1): $$(PREFIX)/$$1/CMakeLists.txt)
endef

# $(EP2134)/libgsl.a: $(EP2134)/libgslcblas.a
define cc_cmake_gsl =
$(eval
$(EP2134)/lib%.a $(EP2134)/%.log: $(EP2134).log
    $(cc_cmake_build)
$(EP2134)/all.log: $(EP2134)/libgsl.a $(EP2134)/libgslcblas.a
all($$1): $(EP2134)/all.log
$(EP2134)/test.log: $(EP2134)/all.log
test($$1): $(EP2134)/test.log)
endef

# $(EP2134)/gsl.lib: $(EP2134)/gslcblas.lib
define msvc_cmake_gsl =
$(call gather,$(addprefix $(P2134)/,gslcblas.lib gsl.lib),)\
$(eval
$(EP2134)/%.lib $(EP2134)/%.log: $(EP213).log
    $(msvc_cmake_build)
$(EP2134)/ALL_BUILD.log: $(EP2134)/gsl.lib $(EP2134)/gslcblas.lib
all($$1): $(EP2134)/ALL_BUILD.log
$(EP2134)/RUN_TESTS.log: $(EP2134)/ALL_BUILD.log
test($$1): $(EP2134)/RUN_TESTS.log)
endef

define msvc_cmake_pthread-win32 =
$(call gather,$(P2134)/pthreadVSE2.lib,)\
$(eval
$(EP2134)/pthreadVSE2.lib: $(EP2134)/pthread-win32.log
$(EP2134)/%.log: $(EP213).log
    $(msvc_cmake_build)
$(EP2134)/ALL_BUILD.log: $(EP2134)/pthreadVSE2.lib
all($$1): $(EP2134)/ALL_BUILD.log
$(EP2134)/RUN_TESTS.log: $(EP2134)/ALL_BUILD.log
test($$1): $(EP2134)/RUN_TESTS.log)
endef

$(call var_base,git://github.com/BrianGladman/gsl.git,,URL:gsl)
$(call var_base,git://github.com/GerHobbelt/pthread-win32.git,,URL:pthread-win32)

# Fixing bug with 'long double' under MinGW gcc
$(call var_reg,-D__USE_MINGW_ANSI_STDIO,$$1,CFLAGS:gsl,gcc gcc-%,%:%)

$(call noop,$(call vect,2 3,%,git(%) cmakelists(%) cmake(%) all(%) test(%),gsl pthread-win32,patsubst))

$(call git,gsl)
$(call cmakelists,gsl)
$(call build,cc_cmake,gsl,$(CC_MATRIX))
$(call build,msvc_cmake,gsl,$(call matrix_trunc,1 2,$(MSVC_MATRIX)))
$(call build,cc_cmake_gsl,gsl,$(CC_MATRIX))
$(call build,msvc_cmake_gsl,gsl,$(MSVC_MATRIX))

$(call git,pthread-win32)
$(call cmakelists,pthread-win32)
$(call build,msvc_cmake,pthread-win32,$(call matrix_trunc,1 2,$(MSVC_MATRIX)))
$(call build,msvc_cmake_pthread-win32,pthread-win32,$(MSVC_MATRIX))

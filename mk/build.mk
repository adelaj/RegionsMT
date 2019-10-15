BUILD_CC_TOOLCHAIN := gcc gcc-% clang clang-% icc

define build_cc_base =
$(call var_base,BUILD_CC_QUERY,$1,$(call transform,$$1:$$(lastword $$(subst :, ,$$(filter $$2%,$$(TOOLCHAIN)))):$$3:$$4,$1),.)\
$(call var_base,BUILD_CC_TOOLCHAIN,$1,$$(word 2,$$(subst :, ,$$(BUILD_CC_QUERY:$$2))),.)\
$(if $(filter $(BUILD_CC_TOOLCHAIN),$(BUILD_CC_TOOLCHAIN:$1)),$(eval
$(call var_base,BUILD_CC_BASE,$1,$$(PREFIX)/$$(firstword $$(subst :, ,$$2))/,.)
$(call gather,GATHER_FILE,$($1))
$(call foreachl,2,gather,BUILD_CC_GATHER_DEP:$1,$(patsubst $(BUILD_CC_BASE:$1)src/%,$(dir $($1))mk/%.mk,$(call fetch_var,CSRC:$(BUILD_CC_QUERY:$1))))
$(call foreachl,2,gather,BUILD_CC_GATHER_OBJ:$1,$(patsubst $(dir $($1))mk/%.mk,$(dir $($1))obj/%.o,$(BUILD_CC_GATHER_DEP:$1)))

.SECONDEXPANSION:
$$(BUILD_CC_GATHER_OBJ$$(COL)$$1): $$(dir $$($$1))obj/%.c.o: $$(BUILD_CC_BASE$$(COL)$$1)src/%.c $$(dir $$($$1))mk/%.c.mk | $$$$(PARENT$$$$(COL)$$$$@) $$(patsubst $$(PERC),header($$(PERC)),$$(call fetch_var2,CINC$$(COL)$$(BUILD_CC_QUERY$$(COL)$$1),CINC$$(COL)$$1))
    $$(strip $(call fetch_var,CC:$(BUILD_CC_TOOLCHAIN:$1)) -MMD -MP -MF$$(word 2,$$^) $(call fetch_var,CFLAGS:$(BUILD_CC_QUERY:$1)) $$(addprefix -I,$$(call nofirstword,$$|)) -o $$@ -c $$<)

$$(BUILD_CC_GATHER_DEP$$(COL)$$1): | $$$$(PARENT$$$$(COL)$$$$@);

$$($$1): $$(BUILD_CC_GATHER_OBJ$$(COL)$$1) $$(call fetch_var2,LDREQ$$(COL)$$(BUILD_CC_QUERY$$(COL)$$1),LDREQ$$(COL)$$1) | $$$$(PARENT$$$$(COL)$$$$@)
    $$(strip $(call fetch_var,LD:$(BUILD_CC_TOOLCHAIN:$1)) $(call fetch_var,LDFLAGS:$(BUILD_CC_QUERY:$1)) -o $$@ $$^)

$(call var_base,GATHER_INC,$(BUILD_CC_GATHER_DEP:$1),.)
$(call var_base,GATHER_FILE,$(BUILD_CC_GATHER_OBJ:$1) $(BUILD_CC_GATHER_DEP:$1),.)))
endef

build_cc = $(call foreachl,1,build_cc_base,$1)

define configure_cmake_base =
$(call var_base,BUILD_CMAKE_QUERY,$1,$(call transform,$$1:$$(lastword $$(subst :, ,$$(filter $$2%,$$(TOOLCHAIN)))):$$3:$$4,$1),.)\
$(call var_base,BUILD_CMAKE_TOOLCHAIN,$1,$$(word 2,$$(subst :, ,$$(BUILD_CMAKE_QUERY:$$2))),.)\
$(if $(filter $(BUILD_CC_TOOLCHAIN),$(BUILD_CMAKE_TOOLCHAIN:$1)),$(eval
$(call gather,GATHER_DIST,$($1))

.SECONDEXPANSION:
$$($$1): | $$$$(PARENT$$$$(COL)$$$$@) configure($$$$@);

.PHONY: $(patsubst %,configure(%),$($1))
$$(patsubst $$(PERC),configure($$(PERC)),$$($$1)): | $$(PREFIX)/$$(firstword $$(subst :, ,$$1))
    cmake \
    -G "Unix Makefiles" \
    -B "$$%" \
    -D CMAKE_C_COMPILER="$(call fetch_var,CC:$(BUILD_CMAKE_TOOLCHAIN:$1))" \
    -D CMAKE_CXX_COMPILER="$(call fetch_var,CXX:$(BUILD_CMAKE_TOOLCHAIN:$1))" \
    -D CMAKE_AR="$(shell which $(call fetch_var,AR:$(BUILD_CMAKE_TOOLCHAIN:$1)))" \
    -D CMAKE_BUILD_TYPE="$(word 4,$(subst :, ,$1))" \
    -D CMAKE_C_FLAGS_INIT="$(call fetch_var,CFLAGS:$(BUILD_CMAKE_QUERY:$1))" \
    -D CMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <OBJECTS>" \
    -D CMAKE_C_ARCHIVE_FINISH="" \
    "$$|"

.PHONY: $(patsubst %,header(%),$($1))
$$(patsubst $$(PERC),header($$(PERC)),$$($$1)): | configure($$$$%)
    cmake --build "$$%" --target copy-headers -- -j -O VERBOSE=1 COLOR=""
))
endef

configure_cmake = $(call foreachl,1,configure_cmake_base,$1)

.PHONY: update(../gsl)
update(../gsl):
    @echo $@ $%

define download_git_base =
$(call gather,GATHER_CONTRIB,$($1))\
$(eval
.SECONDEXPANSION:
$$($$1): | $$$$(PARENT$$$$(COL)$$$$@) update($$$$@);

.PHONY: $(patsubst %,update(%),$($1))

    git clone --depth 1 $(URL:$1) $$@)
endef

download_git = $(call foreachl,1,download_git_base,$1)
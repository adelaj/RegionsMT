BUILD_CC_TOOLCHAIN := gcc gcc-% clang clang-% icc

define build_cc_base =
$(call var_base,BUILD_CC_QUERY,$1,$(call transform,$$1:$$(lastword $$(subst :, ,$$(filter $$2%,$$(TOOLCHAIN)))):$$3:$$4,$1),.)\
$(call var_base,BUILD_CC_TOOLCHAIN,$1,$$(word 2,$$(subst :, ,$$(BUILD_CC_QUERY:$$2))),.)\
$(if $(filter $(BUILD_CC_TOOLCHAIN),$(BUILD_CC_TOOLCHAIN:$1)),$(eval
$(call var_base,BUILD_CC_BASE,$1,$$(PREFIX)/$$(firstword $$(subst :, ,$$2))/,.)
$(call gather,GATHER_FILE,$($1))
$(call foreachl,2,gather,BUILD_CC_GATHER_DEP:$1,$(patsubst $(BUILD_CC_BASE:$1)src/%,$(dir $($1))depend/%.mk,$(call fetch_var,CSRC:$(BUILD_CC_QUERY:$1))))
$(call foreachl,2,gather,BUILD_CC_GATHER_OBJ:$1,$(patsubst $(dir $($1))depend/%.mk,$(dir $($1))obj/%.o,$(BUILD_CC_GATHER_DEP:$1)))

.SECONDEXPANSION:
$$(BUILD_CC_GATHER_OBJ$$(COL)$$1): $$(dir $$($$1))obj/%.c.o: $$(BUILD_CC_BASE$$(COL)$$1)src/%.c $$(dir $$($$1))depend/%.c.mk | $$$$(PARENT$$$$(COL)$$$$@) $$(call fetch_var2,CINC$$(COL)$$(BUILD_CC_QUERY$$(COL)$$1),CINC$$(COL)$$1)
    $$(strip $(call fetch_var,CC:$(BUILD_CC_TOOLCHAIN:$1)) -MMD -MP -MF$$(word 2,$$^) $(call fetch_var,CFLAGS:$(BUILD_CC_QUERY:$1)) $$(addprefix -I,$$(word 2,$$|)) -o $$@ -c $$<)

$$(BUILD_CC_GATHER_DEP$$(COL)$$1): | $$$$(PARENT$$$$(COL)$$$$@);

$$($$1): $$(BUILD_CC_GATHER_OBJ$$(COL)$$1) $$(call fetch_var2,LDREQ$$(COL)$$(BUILD_CC_QUERY$$(COL)$$1),LDREQ$$(COL)$$1) | $$$$(PARENT$$$$(COL)$$$$@)
    $$(strip $(call fetch_var,LD:$(BUILD_CC_TOOLCHAIN:$1)) $(call fetch_var,LDFLAGS:$(QUERY:$1)) -o $$@ $$^)

$(call var_base,GATHER_INC,$(BUILD_CC_GATHER_DEP:$1),.)
$(call var_base,GATHER_FILE,$(BUILD_CC_GATHER_OBJ:$1) $(BUILD_CC_GATHER_DEP:$1),.)))
endef

build_cc = $(call foreachl,1,build_cc_base,$1)

define configure_cmake_base =
$(call var_base,BUILD_CMAKE_QUERY,$1,$(call transform,$$1:$$(lastword $$(subst :, ,$$(filter $$2%,$$(TOOLCHAIN)))):$$3:$$4,$1),.)\
$(call var_base,BUILD_CMAKE_TOOLCHAIN,$1,$$(word 2,$$(subst :, ,$$(BUILD_CMAKE_QUERY:$$2))),.)\
$(if $(filter $(BUILD_CC_TOOLCHAIN),$(BUILD_CMAKE_TOOLCHAIN:$1)),$(eval
$(call gather,GATHER_DIST,$($1))
$(call var_base,BUILD_CMAKE_SRC,$1,$$(PREFIX)/$$(firstword $$(subst :, ,$$2)),.)

.SECONDEXPANSION:
$$($$1): $$(BUILD_CMAKE_SRC) | $$$$(PARENT$$$$(COL)$$$$@)
    cmake \
    -G "Unix Makefiles" \
    -B "$$@" \
    -D CMAKE_C_COMPILER="$(call fetch_var,CC:$(BUILD_CMAKE_TOOLCHAIN:$1))" \
    -D CMAKE_CXX_COMPILER="$(call fetch_var,CXX:$(BUILD_CMAKE_TOOLCHAIN:$1))" \
    -D CMAKE_AR="$(call fetch_var,AR:$(BUILD_CMAKE_TOOLCHAIN:$1))" \
    -D CMAKE_BUILD_TYPE="$(word 4,$(subst :, ,$1))" \
    -D CMAKE_C_FLAGS_INIT="$(call fetch_var,CFLAGS:$(BUILD_CMAKE_QUERY:$1))" \
    -D CMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <OBJECTS>" \
    -D CMAKE_C_ARCHIVE_FINISH="" \
    "$$^" && \
    cmake --build "$$@" --target copy-headers))
endef

define download_git_base =
$(eval
$(call gather,GATHER_DIST,$($1))
.SECONDEXPANSION:
$$($$1): | $$$$(PARENT$$$$(COL)$$$$@)
    git clone --depth 1 $(URL:$1) $$@)
endef
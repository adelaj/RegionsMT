CC_TOOLCHAIN := gcc gcc-% clang clang-% icc

define cc_base =
$(if $(filter $(CC_TOOLCHAIN),$(TOOLCHAIN:$2)),$(eval
$(call gather,GATHER_FILE,$(PREFIX)/$2/$1/$3/$4/$1)
$(call foreachl,2,gather,CC_GATHER_DEP:$1:$2:$3:$4,$(patsubst $(PREFIX)/$1/src/%,$(PREFIX)/$2/$1/$3/$4/mk/%.mk,$(call rwildcard,$(PREFIX)/$1/src/,*.c)))
$(call foreachl,2,gather,CC_GATHER_OBJ:$1:$2:$3:$4,$(patsubst $(PREFIX)/$2/$1/$3/$4/mk/%.mk,$(PREFIX)/$2/$1/$3/$4/obj/%.o,$(CC_GATHER_DEP:$1:$2:$3:$4)))

$(PREFIX)/$2/$1/$3/$4/$1: $(CC_GATHER_OBJ:$1:$2:$3:$4) $(foreach i,$(call fetch_var2,:$1),LDREQ:$1:$(TOOLCHAIN:$2):$3:$4),$($i)) | $(PARENT:$(PREFIX)/$2/$1/$3/$4/$1)
    $(strip $(call fetch_var,LD:$(TOOLCHAIN:$2)) $(call fetch_var,LDFLAGS:$(CC_QUERY:$1:$(TOOLCHAIN:$2):$3:$4)) -o $@ $^)

.SECONDEXPANSION:
$$(CC_GATHER_OBJ$$(COL)$$1): $$(dir $$($$1))obj/%.c.o: $$(CC_BASE$$(COL)$$1)src/%.c $$(dir $$($$1))mk/%.c.mk | $$$$(PARENT$$$$(COL)$$$$@) $$(call fetch_var2,CREQ$$(COL)$$(CC_QUERY$$(COL)$$1),CREQ$$(COL)$$1)
    $$(strip $(call fetch_var,CC:$(CC_TOOLCHAIN:$1)) -MMD -MP -MF$$(word 2,$$^) $(call fetch_var,CFLAGS:$(CC_QUERY:$1)) $$(addprefix -I,$$(call uniq,$$(call nofirstword,$$|))) -o $$@ -c $$<)

$$(CC_GATHER_DEP$$(COL)$$1): | $$$$(PARENT$$$$(COL)$$$$@);

$(call var_base,GATHER_INC,$(CC_GATHER_DEP:$1),.)
$(call var_base,GATHER_FILE,$(CC_GATHER_OBJ:$1) $(CC_GATHER_DEP:$1),.)))
endef

cc = $(call foreachl,1 2 3 4,cc_base,$1,$2,$3,$4)

define cmake_base =
$(call var_base,CMAKE_QUERY,$1,$(call transform,$$1:$$(lastword $$(subst :, ,$$(filter $$2%,$$(TOOLCHAIN)))):$$3:$$4,$1),.)\
$(call var_base,CMAKE_TOOLCHAIN,$1,$$(word 2,$$(subst :, ,$$(CMAKE_QUERY:$$2))),.)\
$(if $(filter $(CC_TOOLCHAIN),$(CMAKE_TOOLCHAIN:$1)),$(eval
$(call gather,GATHER_DIST,$($1))

$($1): | $(PARENT:$($1)) $(PREFIX)/$(firstword $(subst :, ,$1))
    cmake \
    -G "Unix Makefiles" \
    -B "$%" \
    -D CMAKE_MAKE_PROGRAM="$(MAKE)" \
    -D CMAKE_C_COMPILER="$(call fetch_var,CC:$(CMAKE_TOOLCHAIN:$1))" \
    -D CMAKE_CXX_COMPILER="$(call fetch_var,CXX:$(CMAKE_TOOLCHAIN:$1))" \
    -D CMAKE_AR="$(shell which $(call fetch_var,AR:$(CMAKE_TOOLCHAIN:$1)))" \
    -D CMAKE_BUILD_TYPE="$(word 4,$(subst :, ,$1))" \
    -D CMAKE_C_FLAGS_INIT="$(call fetch_var,CFLAGS:$(CMAKE_QUERY:$1))" \
    -D CMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <OBJECTS>" \
    -D CMAKE_C_ARCHIVE_FINISH="" \
    "$(word 2,$|)"
))
endef

cmake = $(call foreachl,1,cmake_base,$1)

define git_base =
$(call gather,GATHER_CONTRIB,$($1))\
$(eval
$($1): | $(PARENT:$($1))
    git clone --depth 1 "$(URL:$1)" $@)
endef

git = $(call foreachl,1,git_base,$1)
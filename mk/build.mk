CC_TOOLCHAIN := gcc gcc-% clang clang-% icc

define cc_base =
$(if $(filter $(CC_TOOLCHAIN),$(TOOLCHAIN:$2)),
$(call gather,GATHER_FILE,$(PREFIX)/$2/$1/$3/$4/$1)
$(call foreachl,2,gather,CC_GATHER_DEP:$1:$2:$3:$4,$(patsubst $(PREFIX)/$1/src/%,$(PREFIX)/$2/$1/$3/$4/mk/%.mk,$(call rwildcard,$(PREFIX)/$1/src/,*.c)))
$(call foreachl,2,gather,CC_GATHER_OBJ:$1:$2:$3:$4,$(patsubst $(PREFIX)/$2/$1/$3/$4/mk/%.mk,$(PREFIX)/$2/$1/$3/$4/obj/%.o,$(CC_GATHER_DEP:$1:$2:$3:$4)))
$(call var_base,GATHER_INC,$(CC_GATHER_DEP:$1:$2:$3:$4),+=)
$(call var_base,GATHER_FILE,$(CC_GATHER_DEP:$1:$2:$3:$4) $(CC_GATHER_OBJ:$1:$2:$3:$4),+=)
$(eval
$$(PREFIX)/$$2/$$1/$$3/$$4/$$1: $$(CC_GATHER_OBJ:$$1:$$2:$$3:$$4) $$(call fetch_var2,LDREQ $$1 $$(TOOLCHAIN:$$2) $$3 $$4,. $$1 $$2 $$3 $$4)
    $$(strip $(call fetch_var,LD $(TOOLCHAIN:$2)) $(call fetch_var,LDFLAGS $1 $(TOOLCHAIN:$2) $3 $4) -o $$@ $$^)

$$(CC_GATHER_OBJ:$$1:$$2:$$3:$$4): $$(PREFIX)/$$2/$$1/$$3/$$4/obj/%.o: $$(PREFIX)/$$1/src/% $$(PREFIX)/$$2/$$1/$$3/$$4/mk/%.mk $$(call fetch_var2,CREQ $$1 $$(TOOLCHAIN:$$2) $$3 $$4,. $$1 $$2 $$3 $$4)
    $$(strip $(call fetch_var,CC $(TOOLCHAIN:$2)) -MMD -MP -MF$$(word 2,$$^) $(call fetch_var,CFLAGS $1 $(TOOLCHAIN:$2) $3 $4) $$(addprefix -I,$$(wordlist 3,$$(words $$(^D)),$$(^D))) -o $$@ -c $$<)
))
endef

cc = $(call foreachl,1 2 3 4,cc_base,$1,$2,$3,$4)
PIPE_TEST := [ $$$${PIPESTATUS[0]} -eq 0 ] || (rm $$@ && false)

define cmake_base =
$(if $(filter $(CC_TOOLCHAIN),$(TOOLCHAIN:$2)),
$(call gather,GATHER_DIST,$(PREFIX)/$2/$1/$3/$4)
$(call gather,GATHER_FILE,$(PREFIX)/$2/$1/$3/$4.log)
$(eval
$$(PREFIX)/$$2/$$1/$$3/$$4.log: $$(PREFIX)/$$1.log
    cmake \
    -G "Unix Makefiles" \
    -B $$(@:%.log=%) \
    -D CMAKE_MAKE_PROGRAM="$(MAKE)" \
    -D CMAKE_C_COMPILER="$(call fetch_var,CC $(TOOLCHAIN:$2))" \
    -D CMAKE_CXX_COMPILER="$(call fetch_var,CXX $(TOOLCHAIN:$2))" \
    -D CMAKE_AR="$(shell which $(call fetch_var,AR $(TOOLCHAIN:$2)))" \
    -D CMAKE_RANLIB="$(shell which $(call fetch_var,RANLIB $(TOOLCHAIN:$2)))" \
    -D CMAKE_BUILD_TYPE="$4" \
    -D CMAKE_C_FLAGS_INIT="$(call fetch_var,CFLAGS $1 $(TOOLCHAIN:$2) $3 $4)" \
    $$(<:%.log=%) \
    | tee $$@; \
    $(PIPE_TEST)
))
endef

cmake = $(call foreachl,1 2 3 4,cmake_base,$1,$2,$3,$4)

define git_base =
$(call gather,GATHER_CONTRIB,$(PREFIX)/$1)
$(call gather,GATHER_FILE,$(PREFIX)/$1.log)
$(eval
$$(PREFIX)/$$1.log:
    $$(if $$(wildcard $$(@:%.log=%)),\
    git -C $$(@:%.log=%) pull --depth 1 | tee $$@,\
    git clone --depth 1 $$(URL:$$(@F:%.log=%)) $$(@:%.log=%) | tee $$@); \
    $(PIPE_TEST)
)
endef

git = $(call foreachl,1,git_base,$1)
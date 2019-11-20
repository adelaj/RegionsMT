# $env:MSYS2_ARG_CONV_EXCL=\*
# $env:MSYS2PATH=C:\msys32\usr\bin\bash.exe -c 'echo $PATH'
# $env:MSYS2WD=C:\msys32\usr\bin\bash.exe -c 'pwd'
# $env:MSYSTEM="MINGW32"
# C:\msys32\usr\bin\bash.exe -l -c 'cd $MSYS2WD; PATH=$MSYS2PATH:$PATH mingw32-make --warn-undefined-variables CLEAN_GROUP=\"1 2\" MATRIX=\"msvc1:Win32:Debug\" TOOLCHAIN=\"msvc1:msvc\" -O -n clean'

build = $(call vect,2 3,$$(eval $$$$(call $1,$$2,$$(subst :,$$(COMMA),$$3))),$2,$3,feval)

P213 = $(PREFIX)/$2/$1/$3
P2134 = $(P213)/$4
EP213 := $$(PREFIX)/$$2/$$1/$$3
EP2134 := $(EP213)/$$4

R123 = $1 $(TOOLCHAIN:$2) $(ARCH:$3)
R1234 = $(R123) $(CONFIG:$4)
ER123 := $$1 $$(TOOLCHAIN:$$2) $$(ARCH:$$3)
ER1234 := $(ER123) $$(CONFIG:$$4)

define cc =
$(eval CC_DEP := $(patsubst $(SRC:$1)/%,$(P2134)/mk/%.mk,$(call rwildcard,$(addsuffix /,$(SRC:$1)),*.c)))\
$(eval CC_OBJ := $(patsubst $(P2134)/mk/%.mk,$(P2134)/obj/%.o,$(CC_DEP)))\
$(call gather,$(P2134)/$1 $(CC_DEP) $(CC_OBJ),)\
$(if $(filter 1,$(CLEAN_GROUP)),$(call clean,$(P2134)/$1 $(CC_DEP) $(CC_OBJ),rm))\
$(eval CC_CREQ := $$(call fetch_var2,CREQ $(ER1234),. $$1 $$2 $$3 $$4))\
$(call var_base,$(CC_DEP),$$1,INCLUDE)\
$(eval
$(EP2134)/$$1: $(CC_OBJ) $$(call fetch_var2,LDREQ $(ER1234),. $$1 $$2 $$3 $$4)
    $$(strip $(call fetch_var,LD $(TOOLCHAIN:$2)) $(call fetch_var,LDFLAGS $(R1234)) -o $$@ $$^ $(call fetch_var,LDLIB $(R1234)))
$(EP2134)/obj/%.o: $(SRC:$1)/% $(EP2134)/mk/%.mk $(CC_CREQ)
    $$(strip $(call fetch_var,CC $(TOOLCHAIN:$2)) -MMD -MP -MF$$(word 2,$$^) $(call fetch_var,CFLAGS $(R1234)) $(addprefix -I,$(CC_CREQ:.log=)) -o $$@ -c $$<)
all($$1): $(EP2134)/$$1
all: | all($$1)
test($$1): $(EP2134)/$$1
test: | test($$1))
endef

on_error = ([ -f "$1" ] && (cat "$1"; mv "$1" "$(1:.log=.error)"; false))

define cc_cmake =
$(call gather,$(addprefix $(P2134),.log .error),)\
$(if $(filter 2,$(CLEAN_GROUP)),\
$(call clean,$(P2134),rm-r)\
$(call clean,$(addprefix $(P2134),.log .error),rm))\
$(eval
$(EP2134).log: $$(PREFIX)/$$1/CMakeLists.txt
    $$(strip $(CMAKE) \
    -G "Unix Makefiles" \
    -D CMAKE_MAKE_PROGRAM="$(MAKE)" \
    -D CMAKE_C_COMPILER="$(call fetch_var,CC $(TOOLCHAIN:$2))" \
    -D CMAKE_CXX_COMPILER="$(call fetch_var,CXX $(TOOLCHAIN:$2))" \
    -D CMAKE_LINKER="$(call fetch_var,LD $(TOOLCHAIN:$2))" \
    -D CMAKE_AR="$(shell which $(call fetch_var,AR $(TOOLCHAIN:$2)))" \
    -D CMAKE_RANLIB="$(shell which $(call fetch_var,RANLIB $(TOOLCHAIN:$2)))" \
    -D CMAKE_BUILD_TYPE="$4" \
    -D CMAKE_C_FLAGS_$(call uc,$4)="$(call fetch_var,CFLAGS $(R1234))" \
    -D CMAKE_EXE_LINKER_FLAGS_$(call uc,$4)="$(call fetch_var,LDFLAGS $(R1234))" \
    -D CMAKE_C_LINK_EXECUTABLE="<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" \
    $(call fetch_var,CMAKEFLAGS $(R1234)) \
    -S $$(<D) \
    -B $$(@:.log=) \
    &> $$@ \
    || $$(call on_error,$$@))
cmake($$1): $(EP2134).log)
endef

cc_cmake_build =\
$$(strip $(CMAKE) \
--build $$(<:.log=) \
--target $$* \
-- -j -O VERBOSE=1 COLOR="" $$(if $$(filter test,$$*),ARGS="--output-on-failure") \
&> $$(basename $$@).log \
|| $$(call on_error,$$(basename $$@).log))

define msvc_cmake =
$(call gather,$(addprefix $(P213),.log .error),)
$(if $(filter 2,$(CLEAN_GROUP)),
$(call clean,$(P213),rm-r)
$(call clean,$(addprefix $(P213),.log .error),rm))
$(eval MSVC_CREQ := $(addprefix /I../../,$(basename $(call fetch_var2,CREQ $(R123),. $1 $2 $3))))
$(eval MSVC_LDREQ_RELEASE := $(patsubst %,../../%,$(call fetch_var2,LDREQ $(R123) Release,. $1 $2 $3 Release)))
$(eval MSVC_LDREQ_DEBUG := $(patsubst %,../../%,$(call fetch_var2,LDREQ $(R123) Debug,. $1 $2 $3 Debug)))
$(eval
$(EP213).log: $$(PREFIX)/$$1/CMakeLists.txt $$(call fetch_var2,CREQ $(ER123),. $$1 $$2 $$3)
    $$(strip powershell "$(CMAKE) \
    -G \"Visual Studio 16 2019\" \
    -A \"$$(@F:.log=)\" \
    -D CMAKE_C_FLAGS_RELEASE=\"$(strip $(call fetch_var,CFLAGS $(R123) Release) $(MSVC_CREQ))\" \
    -D CMAKE_C_FLAGS_DEBUG=\"$(strip $(call fetch_var,CFLAGS $(R123) Debug) $(MSVC_CREQ))\" \
    -D CMAKE_EXE_LINKER_FLAGS_RELEASE=\"$(strip $(call fetch_var,LDFLAGS $(R123) Release) $(MSVC_LDREQ_RELEASE))\" \
    -D CMAKE_EXE_LINKER_FLAGS_DEBUG=\"$(strip $(call fetch_var,LDFLAGS $(R123) Debug) $(MSVC_LDREQ_DEBUG))\" \
    -D CMAKE_STATIC_LINKER_FLAGS_RELEASE=\"$(call fetch_var,ARFLAGS $(R123) Release)\" \
    -D CMAKE_STATIC_LINKER_FLAGS_DEBUG=\"$(call fetch_var,ARFLAGS $(R123) Debug)\" \
    $(call fetch_var,CMAKEFLAGS $(R123)) \
    -S $$(<D) \
    -B $$(@:.log=)" \
    &> $$@ \
    || $$(call on_error,$$@))
cmake($$1): $(EP213).log)
endef

msvc_cmake_build =\
powershell "$(CMAKE) \
--build $$(<:.log=) \
--target $$* \
--config $$(notdir $$(@D)) \
--verbose \
--parallel \
-- $(call fetch_var,MSBUILDFLAGS $1 $(TOOLCHAIN:$2) $3 $4)" \
&> $$(basename $$@).log \
|| $$(call on_error,$$(basename $$@).log)

define git = 
$(call gather,$(addprefix $(PREFIX)/$1,.log .error),)\
$(if $(filter 3,$(CLEAN_GROUP)),\
$(call clean,$(PREFIX)/$1,rm-rf)\
$(call clean,$(addprefix $(PREFIX)/$1,.log .error),rm))\
$(eval
$$(PREFIX)/$$1.log:
    $$(if $$(wildcard $$(@:.log=)),\
    git -C $$(@:.log=) pull --depth 1 &> $$@,\
    git clone --depth 1 $$(URL:$$(@F:.log=)) $$(@:.log=) &> $$@) || $$(call on_error,$$@)
git($$1): $$(PREFIX)/$$1.log)
endef

# $env:MSYS2PATH=C:\msys32\usr\bin\bash.exe -c 'echo $PATH'
# $env:MSYS2WD=C:\msys32\usr\bin\bash.exe -c 'pwd'
# $env:MSYSTEM="MINGW32"
# C:\msys32\usr\bin\bash.exe -l -c 'cd $MSYS2WD; PATH=$MSYS2PATH:$PATH mingw32-make --warn-undefined-variables CLEAN_GROUP=\"1 2\" ARCH=\"Win32 x64\" CFG=\"Debug Release\" TOOLCHAIN=\"msvc1:msvc\" -O -j clean'

build = $(strip $(call vect,2 3,$$$$(call $1,$$2,$$(subst :,$$(COMMA),$$3)),$2,$3,print2))

P213 = $(PREFIX)/$2/$1/$3
P2134 = $(P213)/$4
EP213 := $$(PREFIX)/$$2/$$1/$$3
EP2134 := $(EP213)/$$4

R123 = $1 $(TOOLCHAIN:$2) $(ARCH:$3)
R1234 = $(R123) $(CONFIG:$4)
ER123 := $$1 $$(TOOLCHAIN:$$2) $$(ARCH:$$3)
ER1234 := $(ER123) $$(CONFIG:$$4)

define cc =
$(P2134)/bin/$1
$(call var_base,$(patsubst $(PREFIX)/$1/src/%,$(P2134)/mk/%.mk,$(call rwildcard,$(PREFIX)/$1/src/,*.c)),,CC_DEP)
$(call var_base,$(patsubst $(P2134)/mk/%.mk,$(P2134)/obj/%.o,$(CC_DEP)),,CC_OBJ)
$(call vect,1,$(P2134)/bin/$1 $(CC_DEP) $(CC_OBJ),,gather)
$(if $(filter 1,$(CLEAN_GROUP)),$(call vect,1,$(P2134)/$1 $(CC_DEP) $(CC_OBJ),remove_file,clean))
$(call var_base,$(CC_DEP),$$1,INCLUDE)
$(eval
$(EP2134)/bin/$$1: $(CC_OBJ) $$(call fetch_var2,LDREQ $(ER1234),. $$1 $$2 $$3 $$4)
    $$(strip $(call fetch_var,LD $(TOOLCHAIN:$2)) $(call fetch_var,LDFLAGS $(R1234)) -o $$@ $$^)

$(EP2134)/obj/%.o: $$(PREFIX)/$$1/src/% $(EP2134)/mk/%.mk $$(call fetch_var2,CREQ $(ER1234),. $$1 $$2 $$3 $$4)
    $$(strip $(call fetch_var,CC $(TOOLCHAIN:$2)) -MMD -MP -MF$$(word 2,$$^) $(call fetch_var,CFLAGS $(R1234)) $$(addprefix -I,$$(wordlist 3,$$(words $$^),$$(^:.log=))) -o $$@ -c $$<)
)
endef

on_error = ([ -f "$1" ] && (cat "$1"; rm "$1"; false))

define cc_cmake =
$(P2134).log
$(call gather,$(P2134).log,)
$(if $(filter 2,$(CLEAN_GROUP)),
$(call clean,$(P2134),remove_dir_opaq)
$(call clean,$(P2134).log,remove_file))
$(eval
$(EP2134).log: $$(PREFIX)/$$1/CMakeLists.txt
    $(strip cmake \
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
    > $$@ \
    || $(call on_error,$$@))
)
endef

define cmake_msvc =
$(P213).log
$(call gather,$(P213).log,)
$(if $(filter 2,$(CLEAN_GROUP)),
$(call clean,$(P213),remove_dir_opaq)
$(call clean,$(P213).log,remove_file))
$(eval
$(EP213).log: $$(PREFIX)/$$1/CMakeLists.txt
    $(strip powershell "cmake \
    -G \"Visual Studio 16 2019\" \
    -A \"$$(@F:.log=)\" \
    -D CMAKE_C_FLAGS_RELEASE=\"$(call fetch_var,CFLAGS $(R123) Release)\" \
    -D CMAKE_C_FLAGS_DEBUG=\"$(call fetch_var,CFLAGS $(R123) Debug)\" \
    -D CMAKE_EXE_LINKER_FLAGS_RELEASE="$(call fetch_var,LDFLAGS $(R123) Release)" \
    -D CMAKE_EXE_LINKER_FLAGS_DEBUG="$(call fetch_var,LDFLAGS $(R123) Debug)" \
    -D CMAKE_STATIC_LINKER_FLAGS_RELEASE=\"$(call fetch_var,ARFLAGS $(R123) Release)\" \
    -D CMAKE_STATIC_LINKER_FLAGS_DEBUG=\"$(call fetch_var,ARFLAGS $(R123) Debug)\" \
    $(call fetch_var,CMAKEFLAGS $(R123)) \
    -S $$(<D) \
    -B $$(@:.log=)" \
    > $$@ \
    || $(call on_error,$$@))
)
endef

define git = 
$(strip $(PREFIX)/$1.log
$(call gather,$(PREFIX)/$1.log,)
$(if $(filter 3,$(CLEAN_GROUP)),
$(call clean,$(PREFIX)/$1,remove_dir_opaq_force)
$(call clean,$(PREFIX)/$1.log,remove_file))
$(eval
$$(PREFIX)/$$1.log:
    $$(if $$(wildcard $$(@:.log=)),\
    git -C $$(@:.log=) pull --depth 1 > $$@ | $(call on_error,$$@),\
    git clone --depth 1 $$(URL:$$(@F:.log=)) $$(@:.log=) > $$@) || $(call on_error,$$@)
))
endef

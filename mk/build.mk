# $env:MSYS2PATH=C:\msys32\usr\bin\bash.exe -c 'echo $PATH'
# $env:MSYS2WD=C:\msys32\usr\bin\bash.exe -c 'pwd'
# $env:MSYSTEM="MINGW32"
# C:\msys32\usr\bin\bash.exe -l -c 'cd $MSYS2WD; PATH=$MSYS2PATH:$PATH mingw32-make --warn-undefined-variables CLEAN_GROUP=\"1 2\" ARCH=\"Win32 x64\" CFG=\"Debug Release\" TOOLCHAIN=\"msvc1:msvc\" -O -j clean'

is_cc = $(and $(filter $(CC_TOOLCHAIN),$(TOOLCHAIN:$1)),$(filter $(CC_ARCH),$2))
is_msvc = $(and $(filter $(MSVC_TOOLCHAIN),$(TOOLCHAIN:$1)),$(filter $(MSVC_ARCH),$2))

cc = $(strip $(call vect,1 2 3 4,$1,$2,$3,$4,cc_base))
define cc_base =
$(if $(call is_cc,$2,$3,$4),$(PREFIX)/$2/$1/$3/$4/$1
$(call var_base,$(patsubst $(PREFIX)/$1/src/%,$(PREFIX)/$2/$1/$3/$4/mk/%.mk,$(call rwildcard,$(PREFIX)/$1/src/,*.c)),,CC_DEP:$1:$2:$3:$4)
$(call var_base,$(patsubst $(PREFIX)/$2/$1/$3/$4/mk/%.mk,$(PREFIX)/$2/$1/$3/$4/obj/%.o,$(CC_DEP:$1:$2:$3:$4)),,CC_OBJ:$1:$2:$3:$4)
$(call vect,1,$(PREFIX)/$2/$1/$3/$4/$1 $(CC_DEP:$1:$2:$3:$4) $(CC_OBJ:$1:$2:$3:$4),,gather)
$(if $(filter 1,$(CLEAN_GROUP)),$(call vect,1,$(PREFIX)/$2/$1/$3/$4/$1 $(CC_DEP:$1:$2:$3:$4) $(CC_OBJ:$1:$2:$3:$4),remove_file,clean))
$(call var_base,$(CC_DEP:$1:$2:$3:$4),$$1,INCLUDE)
$(eval
$$(PREFIX)/$$2/$$1/$$3/$$4/$$1: $$(CC_OBJ:$$1:$$2:$$3:$$4) $$(call fetch_var2,LDREQ $$1 $$(TOOLCHAIN:$$2) $$3 $$4,. $$1 $$2 $$3 $$4)
    $$(strip $(call fetch_var,LD $(TOOLCHAIN:$2)) $(call fetch_var,LDFLAGS $1 $(TOOLCHAIN:$2) $3 $4) -o $$@ $$^)

$$(PREFIX)/$$2/$$1/$$3/$$4/obj/%.o: $$(PREFIX)/$$1/src/% $$(PREFIX)/$$2/$$1/$$3/$$4/mk/%.mk $$(call fetch_var2,CREQ $$1 $$(TOOLCHAIN:$$2) $$3 $$4,. $$1 $$2 $$3 $$4)
    $$(strip $(call fetch_var,CC $(TOOLCHAIN:$2)) -MMD -MP -MF$$(word 2,$$^) $(call fetch_var,CFLAGS $1 $(TOOLCHAIN:$2) $3 $4) $$(addprefix -I,$$(wordlist 3,$$(words $$^),$$(^:.log=))) -o $$@ -c $$<)
))
endef

pipe_test = [ $$$${PIPESTATUS[0]} -eq 0 ] || (rm $1 && false)

cmake_cc = $(strip $(call vect,1 2 3 4,$1,$2,$(filter $(CC_ARCH),$3),$4,cmake_cc_base))
define cmake_cc_base =
$(if $(call is_cc,$2,$3,$4),$(PREFIX)/$2/$1/$3/$4.log
$(call gather,$(PREFIX)/$2/$1/$3/$4,)
$(call gather,$(PREFIX)/$2/$1/$3/$4.log,)
$(if $(filter 2,$(CLEAN_GROUP)),
$(call clean,$(PREFIX)/$2/$1/$3/$4,remove_dir_opaq)
$(call clean,$(PREFIX)/$2/$1/$3/$4.log,remove_file))
$(eval
$$(PREFIX)/$$2/$$1/$$3/$$4.log: $$(PREFIX)/$$1/CMakeLists.txt
    $(strip cmake \
    -G "Unix Makefiles" \
    -D CMAKE_MAKE_PROGRAM="$(MAKE)" \
    -D CMAKE_C_COMPILER="$(call fetch_var,CC $(TOOLCHAIN:$2))" \
    -D CMAKE_CXX_COMPILER="$(call fetch_var,CXX $(TOOLCHAIN:$2))" \
    -D CMAKE_LINKER="$(call fetch_var,LD $(TOOLCHAIN:$2))" \
    -D CMAKE_AR="$(shell which $(call fetch_var,AR $(TOOLCHAIN:$2)))" \
    -D CMAKE_RANLIB="$(shell which $(call fetch_var,RANLIB $(TOOLCHAIN:$2)))" \
    -D CMAKE_BUILD_TYPE="$4" \
    -D CMAKE_C_FLAGS_$(call uc,$4)="$(call fetch_var,CFLAGS $1 $(TOOLCHAIN:$2) $3 $4)" \
    -D CMAKE_EXE_LINKER_FLAGS_$(call uc,$4)="$(call fetch_var,LDFLAGS $1 $(TOOLCHAIN:$2) $3 $4)" \
    -D CMAKE_C_LINK_EXECUTABLE="<CMAKE_LINKER> <FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" \
    $(call fetch_var,CMAKEFLAGS $1 $(TOOLCHAIN:$2) $3 $4) \
    -S $$(<D) \
    -B $$(@:.log=) \
    | tee $$@; \
    $(call pipe_test,$$@))
))
endef

cmake_msvc = $(strip $(call vect,1 2 3,$1,$2,$(filter $(MSVC_ARCH),$3),cmake_msvc_base))
define cmake_msvc_base =
$(if $(call is_msvc,$2,$3,$4),$(PREFIX)/$2/$1/$3.log
$(call gather,$(PREFIX)/$2/$1/$3,)
$(call gather,$(PREFIX)/$2/$1/$3.log,)
$(if $(filter 2,$(CLEAN_GROUP)),
$(call clean,$(PREFIX)/$2/$1/$3,remove_dir_opaq)
$(call clean,$(PREFIX)/$2/$1/$3.log,remove_file))
$(eval
$$(PREFIX)/$$2/$$1/$$3.log: $$(PREFIX)/$$1/CMakeLists.txt
    $(strip powershell "cmake \
    -G \"Visual Studio 16 2019\" \
    -A \"$$(@F:.log=)\" \
    -D CMAKE_C_FLAGS_RELEASE=\"$(call fetch_var,CFLAGS $1 $(TOOLCHAIN:$2) $3 Release)\" \
    -D CMAKE_C_FLAGS_DEBUG=\"$(call fetch_var,CFLAGS $1 $(TOOLCHAIN:$2) $3 Debug)\" \
    -D CMAKE_STATIC_LINKER_FLAGS_RELEASE=\"$(call fetch_var,ARFLAGS $1 $(TOOLCHAIN:$2) $3 Release)\" \
    -D CMAKE_STATIC_LINKER_FLAGS_DEBUG=\"$(call fetch_var,ARFLAGS $1 $(TOOLCHAIN:$2) $3 Debug)\" \
    $(call fetch_var,CMAKEFLAGS $1 $(TOOLCHAIN:$2) $3) \
    -S $$(<:.log=) \
    -B $$(@:.log=)" \
    | tee $$@; \
    $(call pipe_test,$$@))
))
endef

git = $(strip $(call vect,1,$1,git_base))
define git_base = 
$(PREFIX)/$1.log
$(call gather,$(PREFIX)/$1,)
$(call gather,$(PREFIX)/$1.log,)
$(if $(filter 3,$(CLEAN_GROUP)),
$(call clean,$(PREFIX)/$1,remove_dir_opaq)
$(call clean,$(PREFIX)/$1.log,remove_file))
$(eval
$$(PREFIX)/$$1.log:
    $$(if $$(wildcard $$(@:.log=)),\
    git -C $$(@:.log=) pull --depth 1 | tee $$@,\
    git clone --depth 1 $$(URL:$$(@F:.log=)) $$(@:.log=) | tee $$@); \
    $(call pipe_test,$$@)
)
endef

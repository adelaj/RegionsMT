$(call var,CC LD,gcc%,gcc%)
$(call var,CXX,gcc%,,g++%)
$(call var,CC CXX LD,clang%,clang%)
$(call var,AR,gcc%,gcc-ar%)
$(call var,AR,clang%,llvm-ar%)

$(call var,CFLAGS,gcc% clang%,%,%,-std=c11 -Wall -mavx)
$(call var,CFLAGS LDFLAGS,gcc% clang%,i386 i686,%,-m32)
$(call var,CFLAGS LDFLAGS,gcc% clang%,x86_64,%,-m64)
$(call var,CFLAGS LDFLAGS,gcc% clang%,%,Release,-O3 -flto)
$(call var,CFLAGS,gcc% clang%,%,Debug,-D_DEBUG -g)
$(call var,CFLAGS LDFLAGS,gcc%,%,Release,-fuse-linker-plugin)
$(call var,CFLAGS LDFLAGS,gcc%,%,Debug,-Og)
$(call var,CFLAGS LDFLAGS,clang%,%,Debug,-O0)
$(call var,LDFLAGS,gcc% clang%,%,%,-mavx)
$(call var,LDFLAGS,clang%,%,Release,-fuse-ld=gold)



CC_INC := gsl


LD_INC := gsl
LD_LIB := m pthread gsl gslcblas

define var3_set = 
$(eval $1 :=) \
$(call foreachl,6 7 8,feval,$1 += $$($$(call rremsuffix,/*,$2/$$6/$$7/$$8)),$2,$3,$4,$5,$3 *,$4 *,$5 *) \
$(eval $1 := $$(strip $$($1)))
endef

EXEC := CC CXX LD AR
INC := CC_INC LD_INC
VAR := $(EXEC) $(INC) CC_OPT LD_OPT LD_LIB

$(call foreachl,2 3 4 5,feval,$$(call rremsuffix,/*,$$2/$$3/$$4/$$5) ?=,$(VAR),$(TOOLCHAIN_BASE) *,$(ARCH) *,$(CFG) *)
$(call foreachl,2 3 4 5,var3_set,BUILD_$$2/$$3/$$4/$$5,$(VAR),$(TOOLCHAIN_BASE),$(ARCH),$(CFG))
$(call foreachl,2 3 4 5,feval,BUILD_$$2/$$3/$$4/$$5 := $$(BUILD_$$2/$$(firstword $$(subst -, ,$$3))/$$4/$$5),$(VAR),$(TOOLCHAIN_VER),$(ARCH),$(CFG))
$(call foreachl,2 3 4 5,feval,BUILD_$$2/$$3/$$4/$$5 := $$(firstword $$(BUILD_$$2/$$3/$$4/$$5)),$(EXEC),$(TOOLCHAIN),$(ARCH),$(CFG))
$(call foreachl,2 3 4 5,feval,BUILD_$$2/$$3/$$4/$$5 := $$(addsuffix -$$4/$$5,$$(addprefix $(BUILD_PATH)/$$3/,$$(BUILD_$$2/$$3/$$4/$$5))),$(INC),$(TOOLCHAIN),$(ARCH),$(CFG))
$(call foreachl,2 3 4 5,feval,BUILD_$$2/$$3/$$4/$$5 := $$(addsuffix $$(patsubst $$(firstword $$(subst -, ,$$3))%,%,$$3),$$(BUILD_$$2/$$3/$$4/$$5)),$(EXEC),$(TOOLCHAIN_VER),$(ARCH),$(CFG))

ifeq ($(ARCH),)
ARCH = x86_64
endif

ifeq ($(CFG),)
CFG = Release
endif

CC = gcc
CC_OPT = -std=c11 -flto -fuse-linker-plugin -Wall -mavx
CC_OPT-i386 = -m32
CC_OPT-Release = -O3
CC_OPT-Debug = -D_DEBUG -Og -ggdb
CC_INC = ../gsl

LD = $(CC)
LD_OPT = -flto -fuse-linker-plugin -mavx
LD_OPT-i386 = -m32
LD_OPT-Release = -O3
LD_OPT-Debug = -Og
LD_LIB = m pthread gsl gslcblas
LD_INC = ../gsl

TARGET = RegionsMT
OBJ_DIR = ./obj
SRC_DIR = ./src

target = $(foreach cfg,$(CFG),$(addsuffix -$(cfg),$(addprefix $1-,$(ARCH))))

.PHONY: all
all: $(call target,build);

.PHONY: clean
clean: $(call target,clean);

rwildcard = $(sort $(wildcard $1$2)) $(foreach d,$(sort $(wildcard $1*)),$(call rwildcard,$d/,$2))
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

define build_var =
BUILD_$1-$2-$3 = $(addsuffix $4,$($1))
ifneq ($($1-$2),)
BUILD_$1-$2-$3 += $(addsuffix $4,$($1-$2))
endif
ifneq ($($1-$3),)
BUILD_$1-$2-$3 += $(addsuffix $4,$($1-$3))
endif
ifneq ($($1-$2-$3),)
BUILD_$1-$2-$3 += $(addsuffix $4,$($1-$2-$3))
endif
endef

define build_obj =
OBJ$1 = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)$1/%.c.o,$(call rwildcard,$(SRC_DIR)/,*.c))
endef

define build_struct =
STRUCT$1 = $(call uniq,$(dir $(OBJ$1)))
endef

define build =
.PHONY: build$1
build$1: | $(STRUCT$1) $(TARGET)$1;
$(STRUCT$1):; mkdir $$@
$(TARGET)$1: $(OBJ$1); $(LD) $(BUILD_LD_OPT$1) -o $$@ $$^ $(addprefix -L,$(BUILD_LD_INC$1)) $(addprefix -l,$(BUILD_LD_LIB$1))
$(OBJ$1): $(OBJ_DIR)$1/%.c.o: $(SRC_DIR)/%.c; $(CC) $(BUILD_CC_OPT$1) $(addprefix -I,$(BUILD_CC_INC$1)) -o $$@ -c $$^
endef

define clean =
.PHONY: clean$1 clean-obj$1 clean-target$1
clean$1: clean-obj$1 clean-target$1;
clean-obj$1:; rm -rf $(OBJ_DIR)$1
clean-target$1:; rm -f $(TARGET)$1
endef

$(foreach arch,$(ARCH),$(foreach cfg,$(CFG),\
$(eval $(call build_obj,-$(arch)-$(cfg)))\
$(eval $(call build_struct,-$(arch)-$(cfg)))\
$(foreach var,CC_OPT LD_OPT LD_LIB,$(eval $(call build_var,$(var),$(arch),$(cfg),)))\
$(foreach var,CC_INC LD_INC,$(eval $(call build_var,$(var),$(arch),$(cfg),-$(arch)-$(cfg))))\
$(eval $(call build,-$(arch)-$(cfg)))\
$(eval $(call clean,-$(arch)-$(cfg)))))

.PHONY: print-%
print-% :; @echo $* = $($*)

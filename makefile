ifeq ($(ARCH),)
ARCH = x86_64
endif

ifeq ($(CFG),)
CFG = Release
endif

CC = gcc
CC_OPT = -std=c11 -mavx -flto -fuse-linker-plugin -Wall
CC_OPT-i386 = -m32
CC_OPT-Release = -O3
CC_OPT-Debug = -D_DEBUG -Og -ggdb
CC_INC = ../gsl

LD = $(CC)
LD_OPT = -mavx -flto -fuse-linker-plugin
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
all: $(call target,$(TARGET));

.PHONY: clean
clean: $(call target,clean);

rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
struct = $(sort $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)-$1/%,$(dir $(call rwildcard,$(SRC_DIR)/,))))
obj = $(sort $(patsubst $(SRC_DIR)/%$2,$(OBJ_DIR)-$1/%$2.o,$(call rwildcard,$(SRC_DIR)/,*$2)))

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

define build =
.PHONY: $1-$2
$1-$2: | $(call struct,$1-$2) $(TARGET)-$1-$2;
$(call struct,$1-$2):; mkdir -p $$@
$(foreach var,CC_OPT LD_OPT LD_LIB,$(eval $(call build_var,$(var),$1,$2,)))
$(foreach var,CC_INC LD_INC,$(eval $(call build_var,$(var),$1,$2,-$1-$2)))
$(TARGET)-$1-$2: $(call obj,$1-$2,.c); $(LD) $(BUILD_LD_OPT-$1-$2) -o $$@ $$^ $(addprefix -L,$(BUILD_LD_INC-$1-$2)) $(addprefix -l,$(BUILD_LD_LIB-$1-$2))
$(call obj,$1-$2,.c): $(OBJ_DIR)-$1-$2/%.c.o: $(SRC_DIR)/%.c; $(CC) $(BUILD_CC_OPT-$1-$2) $(addprefix -I,$(BUILD_CC_INC-$1-$2)) -o $$@ -c $$^
endef

define clean =
.PHONY: clean-$1-$2 clean-obj-$1-$2 clean-target-$1-$2
clean-$1-$2: clean-obj-$1-$2 clean-target-$1-$2;
clean-obj-$1-$2:; rm -rf $(OBJ_DIR)-$1-$2
clean-target-$1-$2:; rm -f $(TARGET)-$1-$2
endef

$(foreach arch,$(ARCH),$(foreach cfg,$(CFG),$(eval $(call build,$(arch),$(cfg)))))
$(foreach arch,$(ARCH),$(foreach cfg,$(CFG),$(eval $(call clean,$(arch),$(cfg)))))

.PHONY: print-%
print-% :; @echo $* = $($*)

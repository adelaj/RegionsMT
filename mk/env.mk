include mk/common.mk

SHELL = bash

TOOLCHAIN ?=
ARCH ?=
CONFIG ?=
MATRIX ?= gcc:$(shell uname -m):Release
PREFIX ?= ..
CLEAN_GROUP ?= 1

FILTER := $(call vect,2 3 4,$$2:$$3:$$4,gcc gcc-% clang clang-% icc,i386 i686 x86_64,Release Debug,print) $(call vect,2 3,msvc:$$2:$$3,Win32 x64,Release Debug,print)

map_add = $(call __map_add,$1,$(subst :, :,$2))
__map_add = $(call __map_def,$1,$(firstword $2),$(call nofirstword,$2))
__map_def = $(call var_base,$(if $3,$(patsubst :%,%,$(call compress,,$3)),$2),,$1:$2)$2

$(call vect,2,$$2 := $$(call vect,2,$$2,$$($$2),map_add),TOOLCHAIN ARCH CONFIG,feval)
matrix_ftr = $(if $(call find_mask,$(foreach i,$(join TOOLCHAIN ARCH CONFIG,$(subst :, :,:$1)),$(call safe_var,$i,$(call nofirstword,$(subst :, ,$i)))),$(FILTER)),$1)

override MATRIX := $(call vect,1,$(MATRIX),matrix_ftr)
override PREFIX := $(lastword $(PREFIX))
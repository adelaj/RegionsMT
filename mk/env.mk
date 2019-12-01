BASH ?= bash
CMAKE ?= cmake
GIT ?= git
EVAV ?=
SHELL := $(BASH)

TOOLCHAIN ?=
ARCH ?=
CONFIG ?=
MATRIX ?= gcc:$(shell uname -m):Release
PREFIX ?= ..
CLEAN ?=

override PREFIX := $(lastword $(PREFIX))

map_add = $(call __map_add,$1,$(subst :, :,$2))
__map_add = $(call __map_def,$1,$(firstword $2),$(call nofirstword,$2))
__map_def = $(if $(call var_base_decl,$(if $3,$(patsubst :%,%,$(call compress,,$3)),$2),,$1:$2),$2)

VAR := TOOLCHAIN ARCH CONFIG
ORDER := $(call vect,2,ORDER,$(join $(call range,1,$(words $(VAR))),$(addprefix :,$(VAR))),map_add)
$(call vect,2,override $$(ORDER:$$2) := $$(call vect,2,$$(ORDER:$$2),$$(call rev,$$(foreach i,$$(MATRIX),$$(patsubst :%,%,$$(word $$2,$$(subst :, :,:$$i)))) $$($$(ORDER:$$2))),map_add),$(ORDER),feval)

matrix_ftr = $(call vect,1,$1,$2,matrix_ftr_base)
matrix_ftr_base = $(if $(call find_mask,$(foreach i,$(join $(foreach j,$(ORDER),$(ORDER:$j)),$(subst :, :,:$1)),$($i)),$2),$1)

matrix_trunc = $(call uniq,$(call vect,2,$1,$2,matrix_trunc_base))
matrix_trunc_base = $(call __matrix_trunc_base,$1,$(subst :, :,:$2))
__matrix_trunc_base = $(patsubst :%,%,$(call compress,,$(foreach i,$1,$(word $i,$2))))

override MATRIX := $(call uniq,$(MATRIX))

CC_TOOLCHAIN := gcc gcc-% mingw clang clang-% icc
CC_ARCH := i386 i686 x86_64
CC_MATRIX := $(call matrix_ftr,$(MATRIX),$(call vect,2 3 4,$$2:$$3:$$4,$(CC_TOOLCHAIN),$(CC_ARCH),Release Debug,print))

MSVC_TOOLCHAIN := msvc
MSVC_ARCH := Win32 x64
MSVC_MATRIX := $(call matrix_ftr,$(MATRIX),$(call vect,2 3 4,$$2:$$3:$$4,$(MSVC_TOOLCHAIN),$(MSVC_ARCH),Release Debug,print))

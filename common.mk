.RECIPEPREFIX +=
PERC := %
COMMA := ,

nofirstword = $(wordlist 2,$(words $1),$1)
nolastword = $(wordlist 2,$(words $1),0 $1)

firstsep = $(if $2,$(firstword $(subst $1, ,$(firstword $2))) $(call firstsep,$1,$(call nofirstword,$2)))
rev = $(if $1,$(call rev,$(call nofirstword,$1)) $(firstword $1))
rremsuffix = $(if $(filter-out $2,$(patsubst %$1,%,$2)),$(call rremsuffix,$1,$(patsubst %$1,%,$2)),$2)
rwildcard = $(sort $(wildcard $(addprefix $1,$2))) $(foreach d,$(sort $(wildcard $1*)),$(call rwildcard,$d/,$2))
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))
inflate = $(if $1,$(call inflate,$(call nofirstword,$1),$(subst $(firstword $1),$(firstword $1) ,$2)),$(wordlist 1,$(words $2),$2 0))
compress = $(if $1,$(firstword $1)$(call compress,$(call nofirstword,$1)))

inc = $(call compress,$(call incl,$(call inflate,0 1 2 3 4 5 6 7 8 9,$1)))
incl = $(call incl$(lastword 0 $1),$(call nolastword,$1))

incl0 = $1 1
incl1 = $1 2
incl2 = $1 3
incl3 = $1 4
incl4 = $1 5
incl5 = $1 6
incl6 = $1 7
incl7 = $1 8
incl8 = $1 9
incl9 = $(call incl,$1) 0

define gather =
$(if $(filter $2,$(GATHER_$1)),,
$(eval
CLEAN-$2 :=
GATHER_$1 += $2
GATHER_CLEAN_$1 += clean-$2
$(eval 
PARENT-$2 := $(addprefix $(BUILD_PATH)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(BUILD_PATH)/%,%,$2))))))
$(if $(PARENT-$2),
CLEAN-$(PARENT-$2) += clean-$2
$(call gather,DIR,$(PARENT-$2)))))
endef

.PHONY: print-%
print-%:; @echo '$* = $($*)'
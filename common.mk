.RECIPEPREFIX +=
PERC := %
COMMA := ,

firstsep = $(if $2,$(firstword $(subst $1, ,$(firstword $2))) $(call firstsep,$1,$(wordlist 2,$(words $2),$2)))
rev = $(if $1,$(call rev,$(wordlist 2,$(words $1),$1)) $(firstword $1))
rremsuffix = $(if $(filter-out $2,$(patsubst %$1,%,$2)),$(call rremsuffix,$1,$(patsubst %$1,%,$2)),$2)
rwildcard = $(sort $(wildcard $(addprefix $1,$2))) $(foreach d,$(sort $(wildcard $1*)),$(call rwildcard,$d/,$2))
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

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
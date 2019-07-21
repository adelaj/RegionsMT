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

awrap = $(call compress,$(call $1,$(call inflate,0 1 2 3 4 5 6 7 8 9,$2),$3))
inc = $(call awrap,__inc,$1,)
dec = $(call awrap,__dec,$1,0)
__inc = $(call __inc$(lastword 0 $1),$(call nolastword,$1))
__dec = $(call __dec$(lastword $1),$(call nolastword,$1),$2)

__inc0 = $1 1
__inc1 = $1 2
__inc2 = $1 3
__inc3 = $1 4
__inc4 = $1 5
__inc5 = $1 6
__inc6 = $1 7
__inc7 = $1 8
__inc8 = $1 9
__inc9 = $(call __inc,$1) 0

__dec0 = $(call __dec,$1,) 9
__dec1 = $(if $2,$1 0)
__dec2 = $1 1
__dec3 = $1 2
__dec4 = $1 3
__dec5 = $1 4
__dec6 = $1 5
__dec7 = $1 6
__dec8 = $1 7
__dec9 = $1 8

argmsk = $(if $(filter $1,$2),,$(COMMA)$$($(call inc,$1))$(call argmsk,$(call inc,$1),$2))
argcnt = $(eval __tmp := 0)$(__argcnt)$(__tmp)
__argcnt = $(if $(filter simple,$(flavor $(call inc,$(__tmp)))),$(eval __tmp := $(call inc,$(__tmp)))$(eval $(value __argcnt)))

foreach1 = $(foreach i,$(argcnt),$(eval __tmp := $$(call $$1$(call argmsk,1,$(call dec,$(argcnt))),$$i))$(__tmp))
foreachn = $(eval __tmp := $(if $(filter 0,$1),,$$(call foreachn,$(call dec,$1),foreach1,$$2$(call argmsk,1,$(argcnt))))$(__tmp))

.PHONY: print-%
print-%:; @echo '$* = $($*)'
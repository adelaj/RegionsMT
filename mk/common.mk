.RECIPEPREFIX +=
PERC := %
COMMA := ,
COL := :
LP := (
RP := )

feval = $(eval $1)
print = $(eval __tmp := $1)$(__tmp)
safe_var = $(if $(filter-out undefined,$(flavor $1)),$($1),$2)
escape_comma = $(subst $(COMMA),$$(COMMA),$(subst $$,$$$$,$1))
nofirstword = $(wordlist 2,$(words $1),$1)
nolastword = $(wordlist 2,$(words $1),0 $1)
stripfirst = $(wordlist 2,$(words 0 $1),0 $1)
striplast = $(wordlist 1,$(words $1),$1 0)
firstsep = $(foreach i,$2,$(firstword $(subst $1, ,$i)))
lastsep = $(foreach i,$2,$(lastword $(subst $1, ,$i)))

rev = $(call stripfirst,$(call __rev,$1))
__rev = $(if $1,$(call __rev,$(call nofirstword,$1)) $(firstword $1))
rremsuffix = $(if $(filter-out $2,$(patsubst %$1,%,$2)),$(call rremsuffix,$1,$(patsubst %$1,%,$2)),$2)
rwildcard = $(call strip,$(call __rwildcard,$1,$2))
__rwildcard = $(sort $(wildcard $(addprefix $1,$2))) $(foreach d,$(sort $(wildcard $1*)),$(call __rwildcard,$d/,$2))
uniq = $(call striplast,$(call __uniq,$1))
__uniq = $(if $1,$(firstword $1) $(call __uniq,$(filter-out $(firstword $1),$1)))
inflate = $(if $1,$(call inflate,$(call nofirstword,$1),$(subst $(firstword $1),$(firstword $1) ,$2)),$(call striplast,$2))
compress = $(if $2,$(firstword $2)$(call __compress,$1,$(call nofirstword,$2)))
__compress = $(if $2,$1$(firstword $2)$(call __compress,$1,$(call nofirstword,$2)))

awrap = $(call compress,,$(call $1,$(call inflate,0 1 2 3 4 5 6 7 8 9,$2),$3))
inc = $(call awrap,__inc,$1,)
incx2 = $(call awrap,__incx2,$1,)
__incx2 = $(call __inc,$(call __inc,$1))
__inc = $(call __inc$(lastword 0 $1),$(call nolastword,$1))
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
dec = $(call awrap,__dec,$1,0)
__dec = $(call __dec$(lastword $1),$(call nolastword,$1),$2)
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

rangel = $(call striplast,$(call __rangel,$1,$2))
__rangel = $(if $(filter-out $1,$2),$1 $(call rangel,$(call inc,$1),$2))
rangeu = $(call stripfirst,$(call __rangeu,$1,$2))
__rangeu = $(if $(filter-out $1,$2),$(call rangeu,$1,$(call dec,$2)) $2)
range = $(call striplast,$1 $(call rangeu,$1,$2))

argmskdu = $(if $(filter $1,$2),,$3$$($(call inc,$1))$(call argmskdu,$(call inc,$1),$2,$3))
argmskd = $$($1)$(call argmskdu,$1,$2,$3)
argmsku = $(call argmskdu,$1,$2,$(COMMA))
argmsk = $$($1)$(call argmsku,$1,$2)

# Warning! There is no reliable way to track trailing empty arguments: unused arguments are not being undefined in the nested function calls
argcnt = $(eval __tmp := 0)$(__argcnt_asc)$(__argcnt_dsc)$(__tmp)
__argcnt_asc = $(if $(filter simple,$(flavor $(call inc,$(__tmp)))),$(eval __tmp := $(call inc,$(__tmp)))$(eval $(value __argcnt_asc)))
__argcnt_dsc = $(if $($(__tmp)),,$(eval __tmp := $(call dec,$(__tmp)))$(eval $(value __argcnt_dsc)))

vect = $(eval __tmp := $$(call __vect,$(strip $(foreach i,$1,$(if $(filter-out 1,$(words $($(call inc,$i)))),$i)))$(call argmsku,1,$(argcnt))))$(strip $(__tmp))

__vect = $(eval __tmp := $(eval __tmp := $(argcnt))$(if $1,\
$$(call __vect_base,$(call inc,$(firstword $1)),$(call nofirstword,$1)$(call argmsku,1,$(__tmp)),__vect),\
$$(call $$($(__tmp))$(call argmsku,1,$(call dec,$(__tmp))))))$(__tmp)

__vect_base = $(foreach i,$(call inc,$1),$(foreach j,$($i),$(eval __tmp := $(eval __tmp := $(argcnt))\
$$(call $$($(__tmp))$(call argmsku,1,$1),$$j$(call argmsku,$i,$(call dec,$(__tmp)))))$(__tmp)))

var = $(eval __tmp := $(eval __tmp := $(argcnt))\
$$(call vect,$(call rangeu,2,$(__tmp)),$(call argmsk,1,$(__tmp)),var_base_decl))
var_decl = $(var)$(__tmp)

var_base = $(eval __tmp := $(call argmskd,3,$(argcnt),$(COL)))\
$(if $(filter undefined,$(flavor $(__tmp))),$(eval __tmp1 := $1)$(eval $$(__tmp) := $$(__tmp1)),$(eval __tmp1 := $2)$(eval $$(__tmp) += $$(__tmp1)))
var_base_decl = $(strip $(var_base)$(__tmp1))

find_mask = $(strip $(foreach i,$2,$(if $(strip $(foreach j,$(join $1,$(subst :, :,:$i)),$(call __find_mask_ftr,$(subst :, ,$j)))),,$i)))
__find_mask_ftr = $(if $(filter $(words $1),2),$(filter-out $(lastword $1),$(firstword $1)),.)

apply_mask = $(eval __tmp := $$(call vect,$(call inc,$(words $1)),$(call compress,$(COMMA),$(call escape_comma,$1)),$$2,__apply_mask))$(__tmp)
__apply_mask = $(eval __tmp := $($($(argcnt))))$(__tmp)

fetch_var2 = $(call apply_mask,$2,$(call find_mask,$1,$(.VARIABLES)))
fetch_var = $(call fetch_var2,$1,$1)

A-Z := A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
a-z := a b c d e f g h i j k l m n o p q r s t u v w x y z
a.A-z.Z := $(join $(a-z),$(addprefix .,$(A-Z)))
A.a-Z.z := $(join $(A-Z),$(addprefix .,$(a-z)))

tr = $(if $1,$(call tr,$(call nofirstword,$1),$(subst $(basename $(firstword $1)),$(subst .,,$(suffix $(firstword $1))),$2)),$2)
uc = $(call tr,$(a.A-z.Z),$1)
lc = $(call tr,$(A.a-Z.z),$1)

print(%):; @echo '$* = $($*)'
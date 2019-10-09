define build_cc_base =
$(eval
$(eval
QUERY$$(COL)$1 := $(call transform,$$1:$$(lastword $$(subst :, ,$$(filter $$2%,$$(TOOLCHAIN)))):$$3:$$4,$1)
GATHER_TARGET$$(COL)$1 :=
GATHER_OBJ$$(COL)$1 :=)
$(call gather,TARGET:$1,$($1))
$(call foreachl,3,proxycall,gather,OBJ:$1$(COMMA)$$3,$(patsubst $(PREFIX)/src/%,$(dir $($1))/obj/%.o,$(call rwildcard,$(PREFIX)/src/,*.c)))

.SECONDEXPANSION:
$$(GATHER_OBJ$$(COL)$$1): $(dir $($1))/obj/%.c.o: $(PREFIX)/src/%.c | $$$$(PARENT$$$$(COL)$$$$@) 
    $(call fetch_var,CC:$(QUERY:$1)) $(call fetch_var,CFLAGS:$(QUERY:$1)) -o $$@ -c $$^

.SECONDEXPANSION:
$$(GATHER_TARGET$$(COL)$$1): $$(GATHER_OBJ$$(COL)$$1) $$(LDREQ$$(COL)$$1) | $$$$(PARENT$$$$(COL)$$$$@)
    $(call fetch_var,LD:$(QUERY:$1)) $(call fetch_var,LDFLAGS:$(QUERY:$1)) -o $$@ $$^

GATHER_CLEAN_FILE += $(GATHER_CLEAN_OBJ:$1) $(GATHER_CLEAN_TARGET:$1))
endef

define build_cc =
$(eval DIR-$1/$2/$3/$4 := $(DIR-$1/$2/$3)/$4) \
$(eval $1/$2/$3/$4 := $$(DIR-$1/$2/$3/$4)/$1) \
$(call build_cc_impl,$1,/$2/$3/$4)
endef

define build_cc_cfg =
$(eval DIR-$1/$2/$3 := $(DIR/$2)/$1-$3) \
$(call gather,DIR,$(DIR-$1/$2/$3)) \
$(call foreachl,4,build_cc,$1,$2,$3,$4)
endef
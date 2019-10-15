$(call var_base,URL,gsl,git://github.com/ampl/gsl.git,.)
$(call download_git,gsl)
$(call var,ARTIFACT,gsl,$(BUILD_CC_TOOLCHAIN),%,%,$$$$(addprefix $$$$(PREFIX)/$$$$3/gsl/$$$$4/$$$$5/,libgsl.a libgslcblas.a))
$(call configure_cmake,$(call var_decl,gsl,$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG),$$(PREFIX)/$$2/$$1/$$3/$$4))

.PHONY: $(DOWNLOAD_GIT)
$(DOWNLOAD_GIT): download-%:
    $(if $(wildcard $(BUILD_PATH)/$*),\
    git -C "$(BUILD_PATH)/$*" reset --hard HEAD &>"$(BUILD_PATH)/$*.log" && \
    git -C "$(BUILD_PATH)/$*" pull &>>"$(BUILD_PATH)/$*.log",\
    git clone --depth 1 "$(URL-$*)" "$(BUILD_PATH)/$*" &>"$(BUILD_PATH)/$*.log")


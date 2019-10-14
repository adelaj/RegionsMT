LIBRARY ?= $(VALID_LIBRARY)
LIBRARY := $(call uniq,$(filter $(VALID_LIBRARY),$(LIBRARY)))

TARGET := $(LIBRARY)

define build_cmake =
$(eval


$(GATHER_PROJ-$1$2$3) : | download-$1
    cmake \
    -G "Unix Makefiles" \
    -B "$(GATHER_PROJ-$1$2$3)" \
    -D CMAKE_C_COMPILER="$(BUILD_CC$2)" \
    -D CMAKE_CXX_COMPILER="$(BUILD_CXX$2)" \
    -D CMAKE_AR="$(BUILD_AR$2)" \
    -D CMAKE_BUILD_TYPE="$4" \
    -D CMAKE_C_FLAGS_INIT="$(BUILD_C_FLAGS$2)" \
    -D CMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <OBJECTS>" \
    -D CMAKE_C_ARCHIVE_FINISH="" \
    "$1" \

GATHER_PROJ += $(GATHER_PROJ-$1$2))
endef

URL-gsl := https://github.com/ampl/gsl

GATHER_CLEAN_PROJ :=
GATHER_CLEAN_SRC :=
GATHER_CLEAN_LOG :=
DOWNLOAD_GIT := $(addprefix download-,$(TARGET))

.PHONY: $(DOWNLOAD_GIT)
$(DOWNLOAD_GIT): download-%:
    $(if $(wildcard $(BUILD_PATH)/$*),\
    git -C "$(BUILD_PATH)/$*" reset --hard HEAD &>"$(BUILD_PATH)/$*.log" && \
    git -C "$(BUILD_PATH)/$*" pull &>>"$(BUILD_PATH)/$*.log",\
    git clone --depth 1 "$(URL-$*)" "$(BUILD_PATH)/$*" &>"$(BUILD_PATH)/$*.log")

$(call gather,LOG,$(BUILD_PATH)/gsl.log)
$(call gather,SRC,$(BUILD_PATH)/gsl)

GATHER_CLEAN_FILE += GATHER_CLEAN_LOG
$(call gather_clean_file)

.PHONY: $(GATHER_CLEAN_SRC) $(GATHER_CLEAN_PROJ)
.SECONDEXPANSION:
$(GATHER_CLEAN_SRC) $(GATHER_CLEAN_PROJ): clean-%: | $$(CLEAN-%)
    $(if $(wildcard $*),rm -rf $*)

.PHONY: all
all: $(call foreachl,2 3 4 5,id,$$($$2/$$3/$$4/$$5),$(TARGET),$(TOOLCHAIN),$(ARCH),$(CFG));

.PHONY: clean
clean: | $(GATHER_CLEAN_LOG) $(call foreachl,2,id,clean-$$(DIR/$$2),$(TOOLCHAIN)) $(call foreachl,2 3 4,id,clean-$$(DIR-$$2/$$3/$$4),$(TARGET),$(TOOLCHAIN),$(ARCH));

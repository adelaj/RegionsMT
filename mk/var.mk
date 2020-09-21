tcv = $(addprefix -,$(call __tcv,$(subst -, ,$1)))
__tcv = $(if $(filter-out 1,$(words $1)),$(lastword $1))
tcn = $(firstword $(subst @, ,$1))$(call tcv,$1)
ftrin = $(call striplast,$(call __ftrin,$1,$2))
__ftrin = $(if $2,$(if $(findstring $1,$(firstword $2)),$(firstword $2)) $(call __ftrin,$1,$(call nofirstword,$2)))
nftrin = $(call striplast,$(call __nftrin,$1,$2))
__nftrin = $(if $2,$(if $(findstring $1,$(firstword $2)),,$(firstword $2)) $(call __nftrin,$1,$(call nofirstword,$2)))

# <VAR>:<TOOLCHAIN>
$(call var_reg,$$$$(call tcn,$$$$2),,CC LD,$(CC_TOOLCHAIN))
$(call var_reg,g++$$$$(call tcv,$$$$2),,CXX,$(filter gcc%,$(CC_TOOLCHAIN)))
$(call var_reg,$$$$(call tcn,$$$$2),,CXX,$(filter clang%,$(CC_TOOLCHAIN)))
$(call var_reg,gcc-ar$$$$(call tcv,$$$$2),,AR,$(filter gcc%,$(CC_TOOLCHAIN)))
$(call var_reg,llvm-ar$$$$(call tcv,$$$$2),,AR,$(filter clang%,$(CC_TOOLCHAIN)))
$(call var_reg,gcc-ranlib$$$$(call tcv,$$$$2),,RANLIB,$(filter gcc%,$(CC_TOOLCHAIN)))
$(call var_reg,llvm-ranlib$$$$(call tcv,$$$$2),,RANLIB,$(filter clang%,$(CC_TOOLCHAIN)))

# <VAR>:<TARGET>:<TOOLCHAIN>:<ARCH>:<CONFIG>
$(call var_reg,-std=c17 -Wall -mavx,$$1,CFLAGS:%,$(CC_TOOLCHAIN),%:%)
$(call var_reg,-m32,$$1,CFLAGS LDFLAGS,%,$(CC_TOOLCHAIN),i386 i686,%)
$(call var_reg,-m64 -mcx16,$$1,CFLAGS LDFLAGS,%,$(CC_TOOLCHAIN),x86_64:%)
$(call var_reg,-O3 -flto,$$1,CFLAGS LDFLAGS,%,$(CC_TOOLCHAIN),%:Release)
$(call var_reg,-D_DEBUG -g,$$1,CFLAGS:%,$(CC_TOOLCHAIN),%:Debug)
# GCC do not have support of LTO under macos: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=48180
$(call var_reg,-fuse-linker-plugin,$$1,CFLAGS LDFLAGS,%,$(call nftrin,@macos,$(filter gcc%,$(CC_TOOLCHAIN))),%:Release)
$(call var_reg,-Og,$$1,CFLAGS LDFLAGS,%,$(filter gcc%,$(CC_TOOLCHAIN)),%:Debug)
$(call var_reg,-O0,$$1,CFLAGS LDFLAGS,%,$(filter clang%,$(CC_TOOLCHAIN)),%:Debug)
$(call var_reg,-mavx,$$1,LDFLAGS:%,$(CC_TOOLCHAIN),%:%)
# LLD does not work properly on macos: http://lists.llvm.org/pipermail/cfe-dev/2019-March/061666.html
$(call var_reg,-fuse-ld=lld,$$1,LDFLAGS:%,$(call nftrin,@macos,$(filter clang%,$(CC_TOOLCHAIN))),%:%)

# '__USE_MINGW_ANSI_STDIO' fixes bug with 'long double' under MinGW
$(call var_reg,-D_UCRT -D__MSVCRT_VERSION__=0x1400 -D_UNICODE -D__USE_MINGW_ANSI_STDIO,$$1,CFLAGS:%,$(call ftrin,@mingw,$(CC_TOOLCHAIN)),%:%)
$(call var_reg,-mcrtdll=ucrt,$$1,LDLIB:%,$(filter gcc@mingw%,$(CC_TOOLCHAIN)),%:%)
# See https://sourceforge.net/p/mingw-w64/mailman/message/36621319/ for the details on how to use clang against ucrt
$(call var_reg,-lucrt,$$1,LDLIB:%,$(filter clang@mingw%,$(CC_TOOLCHAIN)),%:%)
# https://sourceforge.net/p/mingw-w64/mailman/message/33154210/
$(call var_reg,-Wl$$$$(COMMA)--large-address-aware,$$1,LDFLAGS:%,$(call ftrin,@mingw,$(CC_TOOLCHAIN)),i386 i686,%)

# Warning! Add "/D_DLL" if using "/MD" instead of "/MT"
$(call var_reg,/Zc:preprocessor /std:c17 /MP /arch:AVX /volatile:ms /D_UNICODE /D_CRT_SECURE_NO_WARNINGS,$$1,CFLAGS:%:msvc:%:%)
$(call var_reg,/MTd /Od /RTC1 /Zi /FS /D_DEBUG,$$1,CFLAGS:%:msvc:%:Debug)
$(call var_reg,/D_DLL /MD /GL /O2,$$1,CFLAGS:%:msvc:%:Release)
$(call var_reg,/LTCG,$$1,LDFLAGS ARFLAGS,%:msvc:%:Release)
$(call var_reg,/SUBSYSTEM:CONSOLE,$$1,LDFLAGS:%:msvc:%:%)
$(call var_reg,/DEBUG /INCREMENTAL,$$1,LDFLAGS:%:msvc:%:Debug)
$(call var_reg,/INCREMENTAL:NO,$$1,LDFLAGS:%:msvc:%:Release)
$(call var_reg,/LARGEADDRESSAWARE,$$1,LDFLAGS:%:msvc:Win32:%)

$(call var_reg,/p:CharacterSet=Unicode,$$1,MSBUILDFLAGS:%:msvc:%:%)
$(call var_reg,/p:WholeProgramOptimization=true,$$1,MSBUILDFLAGS:%:msvc:%:Release)

# <VAR>:<TOOLCHAIN>
$(call var_reg,$$$$2,,CC LD,gcc gcc-% clang clang-%)
$(call var_reg,gcc,,CC LD,gcc@mingw)
$(call var_reg,g++,,CXX,gcc gcc@mingw)
$(call var_reg,$$$$(2:gcc-%=g++-%),,CXX:gcc-%)
$(call var_reg,$$$$2,,CXX,clang clang-%)
$(call var_reg,clang,,CC LD CXX,clang@mingw)
$(call var_reg,gcc-ar,,AR,gcc gcc@mingw)
$(call var_reg,$$$$(2:gcc-%=gcc-ar-%),,AR:gcc-%)
$(call var_reg,llvm-ar,,AR,clang clang@mingw)
$(call var_reg,$$$$(2:clang-%=llvm-ar-%),,AR:clang-%)
$(call var_reg,gcc-ranlib,,RANLIB,gcc gcc@mingw)
$(call var_reg,$$$$(2:gcc-%=gcc-ranlib-%),,RANLIB:gcc-%)
$(call var_reg,llvm-ranlib,,RANLIB,clang clang@mingw)
$(call var_reg,$$$$(2:clang-%=llvm-ranlib-%),,RANLIB:clang-%)

# <VAR>:<TARGET>:<TOOLCHAIN>:<ARCH>:<CONFIG>
$(call var_reg,-std=c11 -Wall -mavx,$$1,CFLAGS:%,gcc gcc-% gcc@mingw clang clang-% clang@mingw,%:%)
$(call var_reg,-m32,$$1,CFLAGS LDFLAGS,%,gcc gcc-% gcc@mingw clang clang-% clang@mingw,i386 i686,%)
$(call var_reg,-m64 -mcx16,$$1,CFLAGS LDFLAGS,%,gcc gcc-% gcc@mingw clang clang-% clang@mingw,x86_64:%)
$(call var_reg,-O3 -flto,$$1,CFLAGS LDFLAGS,%,gcc gcc-% gcc@mingw clang clang-% clang@mingw,%:Release)
$(call var_reg,-D_DEBUG -g,$$1,CFLAGS:%,gcc gcc-% gcc@mingw clang clang-% clang@mingw,%:Debug)
$(call var_reg,-fuse-linker-plugin,$$1,CFLAGS LDFLAGS,%,gcc gcc-% gcc@mingw,%:Release)
$(call var_reg,-Og,$$1,CFLAGS LDFLAGS,%,gcc gcc-% gcc@mingw,%:Debug)
$(call var_reg,-O0,$$1,CFLAGS LDFLAGS,%,clang clang-% clang@mingw,%:Debug)
$(call var_reg,-mavx,$$1,LDFLAGS:%,gcc gcc-% gcc@mingw clang clang-% clang@mingw,%:%)
$(call var_reg,-fuse-ld=lld,$$1,LDFLAGS:%,clang clang-% clang@mingw,%:%)

# '__USE_MINGW_ANSI_STDIO' fixes bug with 'long double' under MinGW
$(call var_reg,-D_UCRT -D__MSVCRT_VERSION__=0x1400 -D_UNICODE -D__USE_MINGW_ANSI_STDIO,$$1,CFLAGS:%,gcc@mingw clang@mingw,%:%)
$(call var_reg,-mcrtdll=ucrt,$$1,LDLIB:%:gcc@mingw:%:%)
# See https://sourceforge.net/p/mingw-w64/mailman/message/36621319/ for the details on how to use clang against ucrt
$(call var_reg,-lucrt,$$1,LDLIB:%:clang@mingw:%:%)
# https://sourceforge.net/p/mingw-w64/mailman/message/33154210/
$(call var_reg,-Wl$$$$(COMMA)--large-address-aware,$$1,LDFLAGS:%,gcc@mingw clang@mingw,i386 i686,%)

# Warning! Add "/D_DLL" if using "/MD" instead of "/MT"
$(call var_reg,/MP /arch:AVX /volatile:ms /D_UNICODE /D_CRT_SECURE_NO_WARNINGS,$$1,CFLAGS:%:msvc:%:%)
$(call var_reg,/MTd /Od /RTC1 /Zi /FS /D_DEBUG,$$1,CFLAGS:%:msvc:%:Debug)
$(call var_reg,/D_DLL /MD /GL /O2,$$1,CFLAGS:%:msvc:%:Release)
$(call var_reg,/LTCG,$$1,LDFLAGS ARFLAGS,%:msvc:%:Release)
$(call var_reg,/SUBSYSTEM:CONSOLE,$$1,LDFLAGS:%:msvc:%:%)
$(call var_reg,/DEBUG /INCREMENTAL,$$1,LDFLAGS:%:msvc:%:Debug)
$(call var_reg,/INCREMENTAL:NO,$$1,LDFLAGS:%:msvc:%:Release)
$(call var_reg,/LARGEADDRESSAWARE,$$1,LDFLAGS:%:msvc:Win32:%)

$(call var_reg,/p:CharacterSet=Unicode,$$1,MSBUILDFLAGS:%:msvc:%:%)
$(call var_reg,/p:WholeProgramOptimization=true,$$1,MSBUILDFLAGS:%:msvc:%:Release)

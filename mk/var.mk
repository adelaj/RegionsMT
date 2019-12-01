# <VAR>:<TOOLCHAIN>
$(call var_reg,$$$$2,,CC LD,gcc gcc-% clang clang-%)
$(call var_reg,gcc,,CC LD,mingw)
$(call var_reg,g++,,CXX,gcc mingw)
$(call var_reg,$$$$(2:gcc-%=g++-%),,CXX:gcc-%)
$(call var_reg,$$$$2,,CXX,clang clang-%)
$(call var_reg,gcc-ar,,AR,gcc mingw)
$(call var_reg,$$$$(2:gcc-%=gcc-ar-%),,AR:gcc-%)
$(call var_reg,llvm-ar,,AR:clang)
$(call var_reg,$$$$(2:clang-%=llvm-ar-%),,AR:clang-%)
$(call var_reg,gcc-ranlib,,RANLIB,gcc mingw)
$(call var_reg,$$$$(2:gcc-%=gcc-ranlib-%),,RANLIB:gcc-%)
$(call var_reg,llvm-ranlib,,RANLIB:clang)
$(call var_reg,$$$$(2:clang-%=llvm-ranlib-%),,RANLIB:clang-%)

# <VAR>:<TARGET>:<TOOLCHAIN>:<ARCH>:<CONFIG>
$(call var_reg,-std=c11 -Wall -mavx,$$1,CFLAGS:%,gcc gcc-% mingw clang clang-%,%:%)
$(call var_reg,-m32,$$1,CFLAGS LDFLAGS,%,gcc gcc-% mingw clang clang-%,i386 i686,%)
$(call var_reg,-m64,$$1,CFLAGS LDFLAGS,%,gcc gcc-% mingw clang clang-%,x86_64:%)
$(call var_reg,-O3 -flto,$$1,CFLAGS LDFLAGS,%,gcc gcc-% mingw clang clang-%,%:Release)
$(call var_reg,-D_DEBUG -g,$$1,CFLAGS:%,gcc gcc-% mingw clang clang-%,%:Debug)
$(call var_reg,-fuse-linker-plugin,$$1,CFLAGS LDFLAGS,%,gcc gcc-% mingw,%:Release)
$(call var_reg,-Og,$$1,CFLAGS LDFLAGS,%,gcc gcc-% mingw,%:Debug)
$(call var_reg,-O0,$$1,CFLAGS LDFLAGS,%,clang clang-%,%:Debug)
$(call var_reg,-mavx,$$1,LDFLAGS:%,gcc gcc-% mingw clang clang-%,%:%)
$(call var_reg,-fuse-ld=gold,$$1,LDFLAGS:%,clang clang-%,%:Release)

# '__USE_MINGW_ANSI_STDIO' fixes bug with 'long double' under MinGW
$(call var_reg,-D_UNICODE -D__USE_MINGW_ANSI_STDIO,$$1,CFLAGS:%:mingw:%:%)

# Warning! Add "/D_DLL" if using "/MD" instead of "/MT"
$(call var_reg,/MP /arch:AVX /volatile:ms /D_UNICODE /D_CRT_SECURE_NO_WARNINGS,$$1,CFLAGS:%:msvc:%:%)
$(call var_reg,/MTd /Od /RTC1 /Zi /FS /D_DEBUG,$$1,CFLAGS:%:msvc:%:Debug)
$(call var_reg,/MT /GL /O2,$$1,CFLAGS:%:msvc:%:Release)
$(call var_reg,/LTCG,$$1,LDFLAGS ARFLAGS,%:msvc:%:Release)
$(call var_reg,/SUBSYSTEM:CONSOLE,$$1,LDFLAGS:%:msvc:%:%)
$(call var_reg,/DEBUG /INCREMENTAL,$$1,LDFLAGS:%:msvc:%:Debug)
$(call var_reg,/INCREMENTAL:NO,$$1,LDFLAGS:%:msvc:%:Release)

$(call var_reg,/p:CharacterSet=Unicode,$$1,MSBUILDFLAGS:%:msvc:%:%)
$(call var_reg,/p:WholeProgramOptimization=true,$$1,MSBUILDFLAGS:%:msvc:%:Release)

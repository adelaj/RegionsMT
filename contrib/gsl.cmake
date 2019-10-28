# CMake build script for the GNU Scientific Library
# Obtained from https://github.com/ampl/gsl
# Should work with patched vession of https://git.savannah.gnu.org/git/gsl.git

# Examples:
# 1. VS 2015, 32-bit static libs (run cmake from the $build_dir)
#    cmake -G"Visual Studio 14 2015" -DGSL_INSTALL_MULTI_CONFIG=ON \
#          [-DCMAKE_INSTALL_PREFIX=<path>] <path to gsl sources>
#    cmake --build . --config Release [--target gsl|install] [-j 8]
#    ctest [-j <N>] [-VV]
# 2. VS 2017, 64-bit shared libs and dynamic runtime
#    cmake -G"Visual Studio 15 2017 Win64" -DGSL_INSTALL_MULTI_CONFIG=ON \
#          -DBUILD_SHARED_LIBS=ON \
#          <path to gsl sources>
#    cmake --build . --config Release [--target gsl|install] [-j 8]
#    ctest [-j <N>] [-VV]
# 3. Linux (run cmake from $build_dir)
#    cmake <path to gsl sources>
#    make [-j] [gsl|install]
#    ctest [-j <N>] [-VV]

# Use CMake 3.4 for target property WINDOWS_EXPORT_ALL_SYMBOLS.
# Ref: https://blog.kitware.com/create-dlls-on-windows-without-declspec-using-new-cmake-export-all-feature/
cmake_minimum_required(VERSION 3.14)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Assign source to group
function (assign_source_group filter src_dir list)
    unset(_tmp)
    foreach (_s IN ITEMS ${${list}})
        get_filename_component(_f "${_s}" DIRECTORY)
        string(REGEX REPLACE "(/\.)*/+$" "" _f "${filter}/${_f}")
        get_filename_component(_p "${src_dir}/${_s}" ABSOLUTE)
        if (MSVC)
            file(TO_NATIVE_PATH "${_f}" _f)
            source_group("${_f}" FILES ${_p})
        elseif (XCODE)
            string(REPLACE "/" "\\\\" _f "${_f}")
            source_group("${_f}" FILES ${_p})
        endif ()
        set(_tmp ${_tmp} ${_p})
        file(TO_NATIVE_PATH ${_p} _p)
        # message(STATUS "File \"${_p}\" added to the group \"${filter}\"")
    endforeach ()
    set(${list} ${_tmp} PARENT_SCOPE)
endfunction (assign_source_group)

project(GSL)

if (POLICY CMP0054)
    # Only interpret `if` arguments as variables or keywords when unquoted.
    cmake_policy(SET CMP0054 NEW)
endif ()
if (POLICY CMP0053)
    cmake_policy(SET CMP0053 NEW)
endif( )

# Advertise user-settable parameters (and defaults) in cmake-gui
option(BUILD_SHARED_LIBS "Export symbols to library." OFF)
option(GSL_DISABLE_WARNINGS "disable warnings." ON)
# EDIT: controled by CMAKE_C_FLAGS_<CONFIG>
# option(MSVC_RUNTIME_DYNAMIC "Use dynamically-linked runtime: /MD(d)" OFF)
option(GSL_INSTALL_MULTI_CONFIG "Install libraries in lib/<config> directory" OFF)

set(PACKAGE "gsl")
set(PACKAGE_NAME ${PACKAGE})
set(PACKAGE_TARNAME ${PACKAGE})
set(PACKAGE_BUGREPORT "")
set(PACKAGE_URL "")
set(LT_OBJDIR ".libs/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# The following part of config.h is hard to derive from configure.ac.
string(REGEX MATCHALL "[^\n]*\n" CONFIG
"/* Define if you have inline with C99 behavior */
#undef HAVE_C99_INLINE

/* Define to 1 if you have the declaration of `acosh', and to 0 if you don't.
     */
#undef HAVE_DECL_ACOSH

/* Define to 1 if you have the declaration of `asinh', and to 0 if you don't.
     */
#undef HAVE_DECL_ASINH

/* Define to 1 if you have the declaration of `atanh', and to 0 if you don't.
     */
#undef HAVE_DECL_ATANH

/* Define to 1 if you have the declaration of `expm1', and to 0 if you don't.
     */
#undef HAVE_DECL_EXPM1

/* Define to 1 if you have the declaration of `feenableexcept', and to 0 if
     you don't. */
#undef HAVE_DECL_FEENABLEEXCEPT

/* Define to 1 if you have the declaration of `fesettrapenable', and to 0 if
     you don't. */
#undef HAVE_DECL_FESETTRAPENABLE

/* Define to 1 if you have the declaration of `finite', and to 0 if you don't.
     */
#undef HAVE_DECL_FINITE

/* Define to 1 if you have the declaration of `fprnd_t', and to 0 if you
     don't. */
#undef HAVE_DECL_FPRND_T

/* Define to 1 if you have the declaration of `frexp', and to 0 if you don't.
     */
#undef HAVE_DECL_FREXP

/* Define to 1 if you have the declaration of `hypot', and to 0 if you don't.
     */
#undef HAVE_DECL_HYPOT

/* Define to 1 if you have the declaration of `isfinite', and to 0 if you
     don't. */
#undef HAVE_DECL_ISFINITE

/* Define to 1 if you have the declaration of `isinf', and to 0 if you don't.
     */
#undef HAVE_DECL_ISINF

/* Define to 1 if you have the declaration of `isnan', and to 0 if you don't.
     */
#undef HAVE_DECL_ISNAN

/* Define to 1 if you have the declaration of `ldexp', and to 0 if you don't.
     */
#undef HAVE_DECL_LDEXP

/* Define to 1 if you have the declaration of `log1p', and to 0 if you don't.
     */
#undef HAVE_DECL_LOG1P

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
#undef HAVE_DOPRNT

/* Defined if you have ansi EXIT_SUCCESS and EXIT_FAILURE in stdlib.h */
#undef HAVE_EXIT_SUCCESS_AND_FAILURE

/* Defined on architectures with excess floating-point precision */
#undef HAVE_EXTENDED_PRECISION_REGISTERS

/* Define if x86 processor has sse extensions. */
#undef HAVE_FPU_X86_SSE

/* Define to 1 if you have the <ieeefp.h> header file. */
#undef HAVE_IEEEFP_H

/* Define this if IEEE comparisons work correctly (e.g. NaN != NaN) */
#undef HAVE_IEEE_COMPARISONS

/* Define this if IEEE denormalized numbers are available */
#undef HAVE_IEEE_DENORMALS

/* Define if you have inline */
#undef HAVE_INLINE

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the `m' library (-lm). */
#undef HAVE_LIBM

/* Define to 1 if you have the `memcpy' function. */
#undef HAVE_MEMCPY

/* Define to 1 if you have the `memmove' function. */
#undef HAVE_MEMMOVE

/* Define to 1 if you have the <memory.h> header file. */
#undef HAVE_MEMORY_H

/* Define this if printf can handle %Lf for long double */
#undef HAVE_PRINTF_LONGDOUBLE

/* Define to 1 if you have the <stdint.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#undef HAVE_STDLIB_H

/* Define to 1 if you have the `strdup' function. */
#undef HAVE_STRDUP

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#undef HAVE_STRING_H

/* Define to 1 if you have the `strtol' function. */
#undef HAVE_STRTOL

/* Define to 1 if you have the `strtoul' function. */
#undef HAVE_STRTOUL

/* Define to 1 if you have the <sys/stat.h> header file. */
#undef HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/types.h> header file. */
#undef HAVE_SYS_TYPES_H

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define to 1 if you have the `vprintf' function. */
#undef HAVE_VPRINTF

/* Define if you need to hide the static definitions of inline functions */
#undef HIDE_INLINE_STATIC

/* Define to the sub-directory in which libtool stores uninstalled libraries.
     */
#undef LT_OBJDIR

/* Name of package */
#undef PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the home page for this package. */
#undef PACKAGE_URL

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* Defined if this is an official release */
#undef RELEASED

/* Define to 1 if you have the ANSI C header files. */
#undef STDC_HEADERS

/* Version number of package */
#undef VERSION

/* Define to 1 if type `char' is unsigned and you are not using gcc.  */
#ifndef __CHAR_UNSIGNED__
# undef __CHAR_UNSIGNED__
#endif

/* Define to `__inline__' or `__inline' if that's what the C compiler
     calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#undef inline
#endif

/* Define to `unsigned int' if <sys/types.h> does not define. */
#undef size_t

/* Define to empty if the keyword `volatile' does not work. Warning: valid
     code using `volatile' can become incorrect without. Disable with care. */
#undef volatile
")

# Get version numbers and parts of config.h from configure.ac.
file(READ configure.ac LINES)
# Replace semicolons with "<semi>" to avoid CMake messing with them.
string(REPLACE ";" "<semi>" LINES "${LINES}")
# Split into lines keeping newlines to avoid foreach skipping empty ones.
string(REGEX MATCHALL "[^\n]*\n" LINES "${LINES}")
set(ah_command FALSE)
foreach (line "${EXTRA_CONFIG}" ${LINES})
    string(REPLACE ";" "" line "${line}")
    if (ah_command)
        # Do nothing.
    elseif (line MATCHES "AC_INIT[^,]*,\\[(.*)\\].*")
        set(VERSION ${CMAKE_MATCH_1})
        message(STATUS "Got VERSION=${VERSION} from configure.ac")
    elseif (line MATCHES "((GSL|CBLAS)_(CURRENT|REVISION|AGE))=(.*)\n")
        set(${CMAKE_MATCH_1} ${CMAKE_MATCH_4})
        message(STATUS "Got ${CMAKE_MATCH_1}=${CMAKE_MATCH_4} from configure.ac")
    elseif (line MATCHES "AH_BOTTOM\\(\\[(.*)")
        set(ah_command bottom)
        set(line "${CMAKE_MATCH_1}")
    elseif (line MATCHES "AH_VERBATIM[^,]+,(.*)")
        set(ah_command verbatim)
        set(line "${CMAKE_MATCH_1}")
    endif ()
    if (ah_command)
        set(saved_ah_command ${ah_command})
        if (line MATCHES "^\\[(.*)")
            set(line "${CMAKE_MATCH_1}")
        endif ()
        if (line MATCHES "\\]\\)")
            set(ah_command FALSE)
            string(REPLACE "])" "" line "${line}")
        endif ()
        # For some reason CMake may bundle several lines together. Split them too.
        string(REGEX MATCHALL "[^\n]*\n" sublines "${line}")
        set(config_add "")
        foreach (subline ${sublines})
            set(config_add ${config_add} "${subline}")
        endforeach ()
        if (saved_ah_command STREQUAL "verbatim")
            set(CONFIG ${config_add} ${CONFIG})
        else ()
            set(CONFIG ${CONFIG} "\n" ${config_add})
        endif ()
    endif ()
endforeach ()
set(PACKAGE_VERSION ${VERSION})
set(PACKAGE_STRING "${PACKAGE} ${VERSION}")

if (NOT (VERSION MATCHES "\\+"))
    # Defined if this is an official release.
    set(RELEASED /**/)
endif ()

include(CheckLibraryExists)
check_library_exists(m cos "" HAVE_LIBM)
if (HAVE_LIBM)
    set(CMAKE_REQUIRED_LIBRARIES m)
endif ()

include(CheckCSourceCompiles)

# Check for inline. (also check for __forceinline?)
foreach (keyword inline __inline__ __inline)
    check_c_source_compiles("
        static ${keyword} void foo() { return 0; }
        int main() {}" C_HAS_${keyword})
    if (C_HAS_${keyword})
        set(C_INLINE ${keyword})
        break ()
    endif ()
endforeach ()
if (C_INLINE)
    # Check for GNU-style extern inline.
    check_c_source_compiles("
        extern ${C_INLINE} double foo(double x);
        extern ${C_INLINE} double foo(double x) { return x + 1.0; }
        double foo(double x) { return x + 1.0; }
        int main() { foo(1.0); }" C_EXTERN_INLINE)
    if (C_EXTERN_INLINE)
        set(HAVE_INLINE 1)
    else ()
        # Check for C99-style inline.
        check_c_source_compiles("
            extern inline void* foo() { foo(); return &foo; }
            int main() { return foo() != 0; }" C_C99INLINE)
        if (C_C99INLINE)
            set(HAVE_INLINE 1)
            set(HAVE_C99_INLINE 1)
        endif ()
    endif ()
endif ()
if (C_INLINE AND NOT C_HAS_inline)
    set(inline ${C_INLINE})
endif ()

# Checks for header files.
include(CheckIncludeFiles)
foreach (header ieeefp.h dlfcn.h inttypes.h memory.h stdint.h stdlib.h
                                strings.h string.h sys/stat.h sys/types.h)
    string(TOUPPER HAVE_${header} var)
    string(REGEX REPLACE "\\.|/" "_" var ${var})
    check_include_files(${header} ${var})
endforeach ()
check_include_files(stdio.h STDC_HEADERS)

check_include_files(unistd.h HAVE_UNISTD_H)
if (NOT HAVE_UNISTD_H)
    check_include_files(io.h HAVE_IO_H)
endif ()

# Check for IEEE arithmetic interface type.
if (CMAKE_SYSTEM_NAME MATCHES Linux)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES sparc)
        set(HAVE_GNUSPARC_IEEE_INTERFACE 1)
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES powerpc)
        set(HAVE_GNUPPC_IEEE_INTERFACE 1)
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES 86)
        set(HAVE_GNUX86_IEEE_INTERFACE 1)
    endif ()
elseif (CMAKE_SYSTEM_NAME MATCHES SunOS)
    set(HAVE_SUNOS4_IEEE_INTERFACE 1)
elseif (CMAKE_SYSTEM_NAME MATCHES Solaris)
    set(HAVE_SOLARIS_IEEE_INTERFACE 1)
elseif (CMAKE_SYSTEM_NAME MATCHES hpux)
    set(HAVE_HPUX_IEEE_INTERFACE 1)
elseif (CMAKE_SYSTEM_NAME MATCHES Darwin)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES powerpc)
        set(HAVE_DARWIN_IEEE_INTERFACE 1)
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES 86)
        set(HAVE_DARWIN86_IEEE_INTERFACE 1)
    endif ()
elseif (CMAKE_SYSTEM_NAME MATCHES NetBSD)
    set(HAVE_NETBSD_IEEE_INTERFACE 1)
elseif (CMAKE_SYSTEM_NAME MATCHES OpenBSD)
    set(HAVE_OPENBSD_IEEE_INTERFACE 1)
elseif (CMAKE_SYSTEM_NAME MATCHES FreeBSD)
    set(HAVE_FREEBSD_IEEE_INTERFACE 1)
endif ()

# Check for FPU_SETCW.
if (HAVE_GNUX86_IEEE_INTERFACE)
    check_c_source_compiles("
        #include <fpu_control.h>
        #ifndef _FPU_SETCW
        #include <i386/fpu_control.h>
        #define _FPU_SETCW(cw) __setfpucw(cw)
        #endif
        int main() { unsigned short mode = 0 ; _FPU_SETCW(mode); }"
        HAVE_FPU_SETCW)
    if (NOT HAVE_FPU_SETCW)
        set(HAVE_GNUX86_IEEE_INTERFACE 0)
    endif ()
endif ()

# Check for SSE extensions.
if (HAVE_GNUX86_IEEE_INTERFACE)
    check_c_source_compiles("
        #include <stdlib.h>
        #define _FPU_SETMXCSR(cw) asm volatile (\"ldmxcsr %0\" : : \"m\" (*&cw))
        int main() { unsigned int mode = 0x1f80 ; _FPU_SETMXCSR(mode); exit(0); }"
        HAVE_FPU_X86_SSE)
endif ()

# Compiles the source code, runs the program and sets ${VAR} to 1 if the
# return value is equal to ${RESULT}.
macro(check_run_result SRC RESULT VAR)
    set(SRC_FILE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src.c)
    file(WRITE ${SRC_FILE} "${SRC}")
    try_run(RUN_RESULT COMPILE_RESULT ${CMAKE_BINARY_DIR} ${SRC_FILE}
                    CMAKE_FLAGS -DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES})
    if (RUN_RESULT EQUAL ${RESULT})
        set(${VAR} 1)
    endif ()
endmacro()

# Check IEEE comparisons, whether "x != x" is true for NaNs.
check_run_result("
    #include <math.h>
    int main (void)
    {
         int status; double inf, nan;
         inf = exp(1.0e10);
         nan = inf / inf ;
         status = (nan == nan);
         exit (status);
    }" 0 HAVE_IEEE_COMPARISONS)

# Check for IEEE denormalized arithmetic.
check_run_result("
    #include <math.h>
    int main (void)
    {
         int i, status;
         volatile double z = 1e-308;
         for (i = 0; i < 5; i++) { z = z / 10.0 ; };
         for (i = 0; i < 5; i++) { z = z * 10.0 ; };
         status = (z == 0.0);
         exit (status);
    }" 0 HAVE_IEEE_DENORMALS)

# Check for long double stdio.
check_run_result("
    #include <stdlib.h>
    #include <stdio.h>
    int main (void)
    {
        const char * s = \"5678.25\"; long double x = 1.234 ;
        fprintf(stderr,\"%Lg\n\",x) ;
        sscanf(s, \"%Lg\", &x);
        if (x == 5678.25) {exit (0);} else {exit(1); }
    }" 0 HAVE_PRINTF_LONGDOUBLE)

if (NOT CMAKE_COMPILER_IS_GNUCC)
    check_run_result("
        #include <limits.h>
        int main (void) { return CHAR_MIN == 0; }" 1 __CHAR_UNSIGNED__)
endif ()

# Remember to put a definition in config.h.in for each of these.
include(CheckSymbolExists)
check_symbol_exists(EXIT_SUCCESS stdlib.h HAVE_EXIT_SUCCESS)
check_symbol_exists(EXIT_FAILURE stdlib.h HAVE_EXIT_FAILURE)
if (HAVE_EXIT_SUCCESS AND HAVE_EXIT_FAILURE)
    set(HAVE_EXIT_SUCCESS_AND_FAILURE 1)
endif ()
set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE=1")
check_symbol_exists(feenableexcept fenv.h HAVE_DECL_FEENABLEEXCEPT)
check_symbol_exists(fesettrapenable fenv.h HAVE_DECL_FESETTRAPENABLE)
set(CMAKE_REQUIRED_DEFINITIONS "")
check_symbol_exists(hypot math.h HAVE_DECL_HYPOT)
check_symbol_exists(expm1 math.h HAVE_DECL_EXPM1)
check_symbol_exists(acosh math.h HAVE_DECL_ACOSH)
check_symbol_exists(asinh math.h HAVE_DECL_ASINH)
check_symbol_exists(atanh math.h HAVE_DECL_ATANH)
check_symbol_exists(ldexp math.h HAVE_DECL_LDEXP)
check_symbol_exists(frexp math.h HAVE_DECL_FREXP)
check_symbol_exists(fprnd_t float.h HAVE_DECL_FPRND_T)
check_symbol_exists(isinf math.h HAVE_DECL_ISINF)
check_symbol_exists(isfinite math.h HAVE_DECL_ISFINITE)
if (HAVE_IEEEFP_H)
    set(IEEEFP_H ieeefp.h)
endif ()
check_symbol_exists(finite math.h;${IEEEFP_H} HAVE_DECL_FINITE)
check_symbol_exists(isnan math.h HAVE_DECL_ISNAN)

# OpenBSD has a broken implementation of log1p.
if (CMAKE_SYSTEM_NAME MATCHES OpenBSD)
    message("avoiding OpenBSD system log1p - using gsl version")
else ()
    check_symbol_exists(log1p math.h HAVE_DECL_LOG1P)
endif ()

# Check for extended floating point registers.
if (NOT (CMAKE_SYSTEM_PROCESSOR MATCHES "^(sparc|powerpc|hppa|alpha)"))
    set(HAVE_EXTENDED_PRECISION_REGISTERS 1)
endif ()

check_symbol_exists(memcpy string.h HAVE_MEMCPY)
check_symbol_exists(memmove string.h HAVE_MEMMOVE)
check_symbol_exists(strdup string.h HAVE_STRDUP)
check_symbol_exists(strtol stdlib.h HAVE_STRTOL)
check_symbol_exists(strtoul stdlib.h HAVE_STRTOUL)
check_symbol_exists(vprintf stdio.h HAVE_VPRINTF)

# Process config.h using autoconf rules.
#file(STRINGS ${GSL_SOURCE_DIR}/config.h.in CONFIG)
list(LENGTH CONFIG length)
math(EXPR length "${length} - 1")
foreach (i RANGE ${length})
    list(GET CONFIG ${i} line)
    if (line MATCHES "^#( *)undef (.*)\n")
        set(space "${CMAKE_MATCH_1}")
        set(var ${CMAKE_MATCH_2})
        if (NOT DEFINED ${var} OR (var MATCHES "HAVE_.*_H" AND NOT ${var}))
            set(line "/* #${space}undef ${var} */\n")
        else ()
            if ("${${var}}" STREQUAL "/**/" OR "${var}" STREQUAL "inline")
                set(value ${${var}})
            elseif (NOT (var MATCHES ^HAVE OR ${var} EQUAL 0 OR ${var} EQUAL 1))
                set(value \"${${var}}\")
            elseif (${var})
                set(value 1)
            else ()
                set(value 0)
            endif ()
            set(line "#${space}define ${var} ${value}\n")
        endif ()
    elseif (BUILD_SHARED_LIBS AND WIN32 AND line MATCHES "^#define GSL_DISABLE_DEPRECATED .*")
        # Keep deprecated symbols when compiling a Windows DLL to prevent linkage errors.
        set(line "/* #undef GSL_DISABLE_DEPRECATED */\n")
    endif ()
    string(REPLACE "<semi>" ";" line "${line}")
    set(CONFIG_OUT "${CONFIG_OUT}${line}")
endforeach ()

file(WRITE ${GSL_BINARY_DIR}/config.h "${CONFIG_OUT}\n")

string(REPLACE "." ";" VERSION_LIST ${VERSION})
list(GET VERSION_LIST 0 GSL_MAJOR_VERSION)
list(GET VERSION_LIST 1 GSL_MINOR_VERSION)
configure_file(gsl_version.h.in ${GSL_BINARY_DIR}/gsl_version.h)

set(GSL_HEADERS_CONFIG "config.h" "gsl_version.h")

if (HAVE_IO_H)
    file(WRITE ${GSL_BINARY_DIR}/unistd.h "#include <io.h>\n")
    set(GSL_HEADERS_CONFIG ${GSL_HEADERS_CONFIG} "unistd.h")
endif ()

assign_source_group("Header Files" ${GSL_BINARY_DIR} GSL_HEADERS_CONFIG)
set(GSL_SOURCES ${GSL_SOURCES} ${GSL_HEADERS_CONFIG})

add_definitions(-DHAVE_CONFIG_H)

if (GSL_DISABLE_WARNINGS)
    # Disable additional warnings.
    if (MSVC)
        add_definitions(
            /wd4018 /wd4028 /wd4056 /wd4101 /wd4244 /wd4267 /wd4334 /wd4477 /wd4700 /wd4723 /wd4756 /wd4996)
    else ()
        foreach (flag -Wall -Wextra -pedantic)
            string(REPLACE ${flag} "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        endforeach ()
    endif ()
endif ()

# Append compiler flags provided via environment variables.
# E.g.: C_FLAGS="-Werror" cmake -G "Unix Makefiles" ../gsl
foreach( lang C CXX )
    if( DEFINED ENV{${lang}_FLAGS} )
        string( APPEND CMAKE_${lang}_FLAGS " $ENV{${lang}_FLAGS}" )
        message( STATUS "CMAKE_${lang}_FLAGS = ${CMAKE_${lang}_FLAGS}" )
        # set( CMAKE_${lang}_FLAGS ${CMAKE_${lang}_FLAGS} CACHE STRING "Compiler flags" FORCE )
    endif()
endforeach()

enable_testing()

# Adds a GSL test. Usage:
#   add_gsl_test(<exename> <source> ...)
function(add_gsl_test exename)
    if (GSL_DISABLE_TESTS)
        return()
    endif ()
    add_executable(${exename} ${ARGN})
    set_target_properties(${exename} PROPERTIES FOLDER Tests)
    target_link_libraries(${exename} gsl)
    add_test(${exename} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${exename})
endfunction()

macro(get_sources dir line source_var header_var)
    set(${source_var})
    set(${header_var})
    string(REGEX REPLACE ".*_SOURCES[ \t]*=(.*)" "\\1" sources ${line})
    string(REGEX MATCHALL "[^ \t]+" sources ${sources})
    foreach (src ${sources})
        if ((${src} MATCHES "\.h$") AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/${src}"))
            set(${header_var} ${${header_var}} ${src})
        elseif ((${src} MATCHES "\.c$") AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/${src}"))
            set(${source_var} ${${source_var}} ${src})
        endif ()
    endforeach ()
endmacro()

macro(get_headers dir line header_var)
    set(${header_var})
    string(REGEX REPLACE ".*_HEADERS[ \t]*=(.*)" "\\1" headers ${line})
    string(REGEX MATCHALL "[^ \t]+" headers ${headers})
    foreach (hdr ${headers})
        if ((${hdr} MATCHES "\.h$") AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/${hdr}"))
            set(${header_var} ${${header_var}} ${dir}/${hdr})
        endif ()
    endforeach ()
endmacro()

# Get subdirectories from Makefile.am.
file(STRINGS Makefile.am lines REGEX "^SUBDIRS[ \t]*=")
foreach (line ${lines})
    string(REGEX REPLACE "SUBDIRS[ \t]*=(.*)" "\\1" dirs ${line})
    string(REGEX MATCHALL "[^ ]+" dirs ${dirs})
endforeach ()

# Extract sources from automake files and add tests.
foreach (dir "." ${dirs})
    file(STRINGS ${dir}/Makefile.am lines)
    foreach (line ${lines})
        if (line MATCHES "_la_SOURCES[ \t]*=")
            get_sources(${dir} "${line}" SOURCES HEADERS)
            if (dir STREQUAL cblas)
                assign_source_group("Source Files" "${dir}" SOURCES)
                assign_source_group("Header Files" "${dir}" HEADERS)
                add_library(gslcblas ${SOURCES} ${HEADERS})
                target_link_libraries(gslcblas ${CMAKE_REQUIRED_LIBRARIES})
                set_target_properties(gslcblas PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
            else ()
                assign_source_group("Source Files/${dir}" "${dir}" SOURCES)
                assign_source_group("Header Files/${dir}" "${dir}" HEADERS)
                set(GSL_SOURCES ${GSL_SOURCES} ${SOURCES} ${HEADERS})
            endif ()
        elseif (line MATCHES "^test.*_SOURCES[ \t]*=")
            get_sources(${dir} "${line}" SOURCES HEADERS)
            assign_source_group("Source Files" "${dir}" SOURCES)
            assign_source_group("Header Files" "${dir}" HEADERS)
            string(REGEX REPLACE "(.*)_SOURCES.*" "\\1" suffix ${line})
            add_gsl_test("${dir}_${suffix}" ${SOURCES} ${HEADERS})
        elseif (line MATCHES "pkginclude_HEADERS[ \t]*=")
            get_headers(${dir} "${line}" HEADERS)
            set(GSL_HEADER_PATHS ${GSL_HEADER_PATHS} ${HEADERS})
        endif ()
    endforeach ()
endforeach ()

# Adding the "fp-win.c" from Brian Gladman's repository
if (MSVC)
    FILE(GLOB_RECURSE SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "fp-win.c")
    if (SOURCES)
        list(GET SOURCES 0 SOURCES)
        message(STATUS "Adding ${SOURCES} to the source tree")
        get_filename_component(dir ${SOURCES} DIRECTORY)
        get_filename_component(SOURCES ${SOURCES} NAME)
        assign_source_group("Source Files/ieee-utils" "${dir}" SOURCES)
        set(GSL_SOURCES ${GSL_SOURCES} ${SOURCES})
        message(STATUS "Removing ieee-utils/fp.c from the source tree")
        list(REMOVE_ITEM GSL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ieee-utils/fp.c")
    endif ()
endif ()
    
# Copying headers
file(MAKE_DIRECTORY gsl)
set(GSL_HEADERS_API)
foreach (path ${GSL_HEADER_PATHS})
    get_filename_component(filename ${path} NAME)
    if (((MSVC) AND (HAVE_C99_INLINE)) AND (filename STREQUAL "gsl_inline.h"))
        message(STATUS "Replacing \"inline\" with \"static inline\" in ${filename}")
        file(READ ${path} gsl_inline)
        string(REGEX REPLACE "(#[ ]*define[ ]*INLINE_[^ ]+[ ]*)(inline)" "\\1static \\2" gsl_inline "${gsl_inline}")
        file(WRITE ${GSL_BINARY_DIR}/gsl/${filename} ${gsl_inline})
    else ()
        configure_file(${path} ${GSL_BINARY_DIR}/gsl/${filename} COPYONLY)
    endif ()
    set(GSL_HEADERS_API ${GSL_HEADERS_API} gsl/${filename})
endforeach ()
assign_source_group("Header Files" ${GSL_BINARY_DIR} GSL_HEADERS_API)
set(GSL_SOURCES ${GSL_SOURCES} ${GSL_HEADERS_API})

if (BUILD_SHARED_LIBS)
    include(CheckCCompilerFlag)
    check_c_compiler_flag(-fPIC HAVE_FPIC)
    if (HAVE_FPIC)
        add_definitions(-fPIC)
    endif ()
    if (WIN32)
        # only needed for global variables that must be exported.  Exporting of
        # all functions is done with target property WINDOWS_EXPORT_ALL_SYMBOLS.
        add_definitions(-DGSL_DLL)
    endif()
endif ()

add_library(gsl ${GSL_SOURCES})
set_target_properties(gsl
    PROPERTIES
        COMPILE_DEFINITIONS DLL_EXPORT
        WINDOWS_EXPORT_ALL_SYMBOLS ON )
target_link_libraries(gsl gslcblas)
target_include_directories(gslcblas
    PUBLIC 
        $<BUILD_INTERFACE:${GSL_BINARY_DIR}>
        $<BUILD_INTERFACE:${GSL_SOURCE_DIR}> )
target_include_directories(gsl 
    PUBLIC $<BUILD_INTERFACE:${GSL_BINARY_DIR}>  )

if (GSL_INSTALL OR NOT DEFINED GSL_INSTALL)
    if (MSVC AND GSL_INSTALL_MULTI_CONFIG)   
        install(TARGETS gsl gslcblas
            EXPORT "gsl-targets"
            CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
            LIBRARY DESTINATION lib/$<CONFIG>
            RUNTIME DESTINATION bin/$<CONFIG>
            ARCHIVE DESTINATION lib/$<CONFIG>)
        # For Debug under VS, also install the pdb files.
        install(
            FILES 
                ${PROJECT_BINARY_DIR}/bin/Debug/gsl.pdb
                ${PROJECT_BINARY_DIR}/bin/Debug/gslcblas.pdb
            CONFIGURATIONS Debug 
            DESTINATION bin/$<CONFIG>)
    else ()
        install(TARGETS gsl gslcblas
            EXPORT "gsl-targets"
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
            ARCHIVE DESTINATION lib)
    endif ()
    install(FILES ${GSL_HEADER_PATHS} DESTINATION include/gsl)
    #set(PC_FILE ${CMAKE_BINARY_DIR}/gsl.pc)
    #configure_file("gsl.pc.cmake" ${PC_FILE} @ONLY)
    #install(FILES ${PC_FILE} DESTINATION lib/pkgconfig)

    # Read config file and replace placeholders.
    #file(READ "${CMAKE_CURRENT_SOURCE_DIR}/gsl-config.in" GSL_CONFIG_FILE)
    #string(REPLACE ";" "@SEMICOLON@" GSL_CONFIG_FILE "${GSL_CONFIG_FILE}")
    # Replace
    #string(REPLACE @prefix@ ${CMAKE_INSTALL_PREFIX} GSL_CONFIG_FILE "${GSL_CONFIG_FILE}")
    #string(REPLACE @exec_prefix@ ${CMAKE_INSTALL_PREFIX}/bin GSL_CONFIG_FILE ${GSL_CONFIG_FILE})
    #string(REPLACE @includedir@ ${CMAKE_INSTALL_PREFIX}/include GSL_CONFIG_FILE "${GSL_CONFIG_FILE}")
    #string(REPLACE @VERSION@ ${VERSION} GSL_CONFIG_FILE "${GSL_CONFIG_FILE}")
    #string(REPLACE @GSL_CFLAGS@ "${CMAKE_C_FLAGS_RELEASE}" GSL_CONFIG_FILE "${GSL_CONFIG_FILE}")
    #string(REPLACE @GSL_LIBS@ "-lgsl -lgslcblas" GSL_CONFIG_FILE "${GSL_CONFIG_FILE}")
    #string(REPLACE @GSL_LIBM@ "-lm" GSL_CONFIG_FILE "${GSL_CONFIG_FILE}")

    # write after putting back ;
    #string(REPLACE "@SEMICOLON@"  "\\;" GSL_CONFIG_FILE "${GSL_CONFIG_FILE}")
    #file(WRITE ${CMAKE_BINARY_DIR}/gsl-config ${GSL_CONFIG_FILE})

    # Install gsl-config
    install(PROGRAMS ${CMAKE_BINARY_DIR}/gsl-config DESTINATION bin)
    
    # Create cmake files that can be used to import GSL into another project. 
    include(CMakePackageConfigHelpers)
    install( 
        EXPORT gsl-targets
        DESTINATION lib/cmake/${PACKAGE_NAME}-${PACKAGE_VERSION}
        NAMESPACE GSL:: )
    #configure_package_config_file(
    #    ${CMAKE_SOURCE_DIR}/cmake/${PACKAGE_NAME}-config.cmake.in
    #    ${CMAKE_BINARY_DIR}/cmake/${PACKAGE_NAME}-config.cmake
    #    INSTALL_DESTINATION cmake/${PACKAGE_NAME} )
    write_basic_package_version_file(
        ${CMAKE_BINARY_DIR}/cmake/${PACKAGE_NAME}-config-version.cmake
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY SameMajorVersion )
    install(
        FILES
            ${CMAKE_BINARY_DIR}/cmake/${PACKAGE_NAME}-config.cmake
            ${CMAKE_BINARY_DIR}/cmake/${PACKAGE_NAME}-config-version.cmake
        DESTINATION 
            lib/cmake/${PACKAGE_NAME}-${PACKAGE_VERSION} )

endif ()

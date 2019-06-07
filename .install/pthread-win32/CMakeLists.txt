# This file is based on 
#   https://github.com/martell/pthreads-win32.cmake/blob/master/CMakeLists.txt
#
# Only supports the creation of static library

cmake_minimum_required(VERSION 3.14)

macro(msvc_set_static_runtime)
    set(_vars CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO)
    foreach(_var IN ITEMS ${_vars})
        string(REGEX REPLACE "/MD" "/MT" ${_var} "${${_var}}")
    endforeach()
endmacro()

set(target pthread-win32)
set(out_prefix pthread)

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    set(MSVC ON)
endif()

project(${target})
option(PTHREAD_SINGLE_FILE "All source files are aggregated into a single translation unit" ON)

enable_language(C)

msvc_set_static_runtime()

if(PTHREAD_SINGLE_FILE)
    set(pthread_sources ${CMAKE_CURRENT_SOURCE_DIR}/pthread.c)
else()
    file(GLOB pthread_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
    list(REMOVE_ITEM pthread_sources ${CMAKE_CURRENT_SOURCE_DIR}/pthread.c)
endif()
source_group("Source Files" FILES ${pthread_sources})

file(GLOB pthread_resources ${CMAKE_CURRENT_SOURCE_DIR}/*.rc)
source_group("Resources" FILES ${pthread_resources})

set(pthread_headers_api_names pthread.h sched.h semaphore.h config.h)
file(GLOB pthread_headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
foreach(header ${pthread_headers_api_names})
    list(REMOVE_ITEM pthread_headers ${CMAKE_CURRENT_SOURCE_DIR}/${header})
    set(pthread_headers_api ${pthread_headers_api} ${CMAKE_CURRENT_BINARY_DIR}/${header})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${header} ${CMAKE_CURRENT_BINARY_DIR}/${header} COPYONLY)
endforeach()
source_group("Header Files" FILES ${pthread_headers})

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DPTW32_ARCHx64)
else()
    add_definitions(-DPTW32_ARCHx86)
endif()

if(MSVC)
    add_definitions(-DPTW32_RC_MSC)
    if (NOT DEFINED PTHREAD_EXCEPTION_SCHEME)
        set(PTHREAD_EXCEPTION_SCHEME SE)
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4311 /wd4312")
    set(pthread_compiler V)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    set(pthread_compiler G)
endif()

if(NOT DEFINED PTHREAD_EXCEPTION_SCHEME)
    set(PTHREAD_EXCEPTION_SCHEME C)
endif()  

if(${PTHREAD_EXCEPTION_SCHEME} MATCHES SE)
    add_definitions(-D__CLEANUP_SEH)
elseif(${PTHREAD_EXCEPTION_SCHEME} MATCHES CE)
    add_definitions(-D__CLEANUP_CXX)
elseif(${PTHREAD_EXCEPTION_SCHEME} MATCHES C)
    add_definitions(-D__CLEANUP_C)
endif()

set(output_name "${out_prefix}${pthread_compiler}${PTHREAD_EXCEPTION_SCHEME}")

add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DPTW32_STATIC_LIB)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(${target} STATIC ${pthread_sources} ${pthread_headers} ${pthread_headers_api} ${pthread_resources})
set_target_properties(${target} PROPERTIES OUTPUT_NAME ${output_name})
set_target_properties(${target} PROPERTIES COMPILE_PDB_NAME ${output_name})
set_target_properties(${target} PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
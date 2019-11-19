# This file is based on 
#   https://github.com/martell/pthreads-win32.cmake/blob/master/CMakeLists.txt
# Run sample
#   cmake -D CMAKE_GENERATOR_PLATFORM="x64" ../pthread-win32

cmake_minimum_required(VERSION 3.14)

macro(pthread_init_tests dir)
    file(GLOB pthread_test_sources ${dir}/*.c)
    source_group("Source Files" FILES ${pthread_test_sources})
    list(REMOVE_ITEM pthread_test_sources ${dir}/benchlib.c ${dir}/wrapper4tests_1.c)
    file(GLOB pthread_test_headers ${dir}/*.h)
    source_group("Header Files" FILES ${pthread_test_headers})
    list(REMOVE_ITEM pthread_test_headers ${dir}/benchtest.h)
    add_library(benchlib STATIC ${dir}/benchlib.c ${dir}/benchtest.h ${pthread_test_headers} ${pthread_headers_api})
    set_target_properties(benchlib PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} FOLDER Tests)
    foreach(_test ${pthread_test_sources})
        get_filename_component(_tmp ${_test} NAME_WE)
        add_executable(${_tmp} ${_test} ${pthread_test_headers} ${pthread_headers_api})
        if(${_tmp} MATCHES "benchtest")
            target_sources(${_tmp} PUBLIC ${dir}/benchtest.h)
            target_link_libraries(${_tmp} PUBLIC benchlib)
        elseif(${_tmp} MATCHES "cancel9")
            target_link_libraries(${_tmp} PRIVATE "ws2_32")
        elseif(${_tmp} MATCHES "sequence2")
            target_sources(${_tmp} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/implement.h)
        elseif(${_tmp} MATCHES "openmp1")
            if (OpenMP_FOUND)
                target_link_libraries(${_tmp} PRIVATE "${OpenMP_C_FLAGS}")
                target_compile_options(${_tmp} PRIVATE "${OpenMP_C_FLAGS}")
            endif()
        endif()
        set_target_properties(${_tmp} PROPERTIES FOLDER Tests)
        target_link_libraries(${_tmp} PUBLIC ${target})
        add_test(NAME ${_tmp} COMMAND $<TARGET_FILE:${_tmp}> WORKING_DIRECTORY $<TARGET_FILE_DIR:${_tmp}>)
    endforeach()
endmacro()

set(target pthread-win32)
set(out_prefix pthread)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
enable_testing()

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    set(MSVC ON)
endif()

project(${target} LANGUAGES C)
option(PTHREAD_MONOLITHIC_SOURCE "All source files are aggregated into a single translation unit" ON)
option(BUILD_SHARED_LIBS "Build shared library" OFF)

enable_language(C)
find_package(OpenMP) # Required only for testing

if(PTHREAD_MONOLITHIC_SOURCE)
    set(pthread_sources ${CMAKE_CURRENT_SOURCE_DIR}/pthread.c)
else()
    file(GLOB pthread_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
    list(REMOVE_ITEM pthread_sources ${CMAKE_CURRENT_SOURCE_DIR}/pthread.c)
endif()
source_group("Source Files" FILES ${pthread_sources})

file(GLOB pthread_resources ${CMAKE_CURRENT_SOURCE_DIR}/*.rc)
source_group("Resource Files" FILES ${pthread_resources})

set(pthread_headers_api_names pthread.h sched.h semaphore.h)
unset(pthread_headers_api)
foreach(header ${pthread_headers_api_names})
    set(pthread_headers_api ${pthread_headers_api} ${CMAKE_CURRENT_SOURCE_DIR}/${header})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${header} ${CMAKE_CURRENT_BINARY_DIR}/${header} COPYONLY)
endforeach()
file(GLOB pthread_headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group("Header Files" FILES ${pthread_headers})

if(${CMAKE_GENERATOR_PLATFORM} MATCHES "x64")
    add_definitions(-DPTW32_ARCHx64)
elseif(${CMAKE_GENERATOR_PLATFORM} MATCHES "Win32")
    add_definitions(-DPTW32_ARCHx86)
else()
    message(ERROR "Unrecognized platform \"${CMAKE_GENERATOR_PLATFORM}\"")
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
else()
    message(ERROR "Unrecognized compiler")
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

set(PTHREADS_COMPATIBILITY_VERSION 2)
set(output_name "${out_prefix}${pthread_compiler}${PTHREAD_EXCEPTION_SCHEME}${PTHREADS_COMPATIBILITY_VERSION}")

add_definitions(-DHAVE_CONFIG_H)

if(BUILD_SHARED_LIBS)
    remove_definitions(-DPTW32_STATIC_LIB)
else()
    add_definitions(-DPTW32_STATIC_LIB)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(${target} ${pthread_sources} ${pthread_headers} ${pthread_resources})
set_target_properties(${target} PROPERTIES OUTPUT_NAME ${output_name} COMPILE_PDB_NAME ${output_name} COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

pthread_init_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests)
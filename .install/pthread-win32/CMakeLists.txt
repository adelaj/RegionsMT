# This file is based on 
#   https://github.com/martell/pthreads-win32.cmake/blob/master/CMakeLists.txt
# Only supports the creation of static library

cmake_minimum_required(VERSION 3.14)

set(target pthreads-win32)
enable_language(C)

project(${target})

set(pthread_api ${CMAKE_CURRENT_SOURCE_DIR}/pthread.h ${CMAKE_CURRENT_SOURCE_DIR}/sched.h ${CMAKE_CURRENT_SOURCE_DIR}/semaphore.h)
source_group("API Files" FILES ${pthread_api})

file(GLOB pthread_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
source_group("Source Files" FILES ${pthread_sources})

set(pthread_resources ${CMAKE_CURRENT_SOURCE_DIR}/version.rc)
set(pthread_headers ${CMAKE_CURRENT_SOURCE_DIR}/config.h ${CMAKE_CURRENT_SOURCE_DIR}/implement.h ${CMAKE_CURRENT_SOURCE_DIR}/need_errno.h)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	add_definitions(-DPTW32_ARCH="x64")
else(CMAKE_SIZEOF_VOID_P EQUAL 8 )
	add_definitions(-DPTW32_ARCH="x86")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

if(MSVC)
    add_definitions(-DPTW32_RC_MSC)
    set(pthread_exception_scheme "SE")
    set(pthread_compiler V)
elseif(CMAKE_COMPILER_IS_GNUCXX)
	set(pthread_compiler G)
endif()

if(PTHREAD_CLEANUP EQUAL "SE")
	add_definitions(-D__CLEANUP_SEH)
elseif(pthread_exception_scheme EQUAL "CE")
    add_definitions(-D__CLEANUP_CXX)	
elseif(pthread_exception_scheme EQUAL "C")
    add_definitions(-D__CLEANUP_C)
endif()

set(target_name "PTHREAD${pthread_compiler}${pthread_exception_scheme})

add_definitions(-DHAVE_PTW32_CONFIG_H)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(${target_name} STATIC ${pthread_sources} ${pthread_api} ${pthread_resources})
set_target_properties(${target_name} PROPERTIES PUBLIC_HEADER "${pthread_api}")
install(TARGETS ${target_name} PUBLIC_HEADER DESTINATION . COMPONENT dev)
cmake_minimum_required(VERSION 3.14)
execute_process(COMMAND ${CMAKE_COMMAND} -D "RDIR=${CMAKE_CURRENT_SOURCE_DIR}/src" -P "scan.cmake" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
include("sources.cmake")
add_executable(RegionsMT ${sources})
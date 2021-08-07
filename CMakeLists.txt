cmake_minimum_required(VERSION 3.14)

include(FetchContent)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(smokylab LANGUAGES C CXX)

message(STATUS "Sdfasdf ${CMAKE_SOURCE_DIR}")

set(SMOKYLAB_ROOT "${CMAKE_SOURCE_DIR}")
set(SMOKYLAB_DEP_DIR "${CMAKE_SOURCE_DIR}/dep")
set(SMOKYLAB_SCRIPT_DIR "${CMAKE_SOURCE_DIR}/scripts")

add_subdirectory(src)

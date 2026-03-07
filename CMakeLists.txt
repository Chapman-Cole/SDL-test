cmake_minimum_required(VERSION 4.1.0)
project(SDL-test LANGUAGES C CXX)

# Find all the necessary dependencies first
include(cmake/find_SDL3.cmake)
include(cmake/find_VulkanComponents.cmake)
include(cmake/find_cglm.cmake)

# Place all executables and dlls in a bin folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/static$<0:>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/shared$<0:>")

add_subdirectory(engine)
add_subdirectory(examples)
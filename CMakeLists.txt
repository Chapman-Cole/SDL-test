cmake_minimum_required(VERSION 4.1.0)
project(SDL-test LANGUAGES C CXX)

# Find all the necessary dependencies first
include(cmake/find_SDL3.cmake)
include(cmake/find_VulkanComponents.cmake)
include(cmake/find_cglm.cmake)

# Place all executables and dlls in a bin folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib$<0:>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib$<0:>")

option(USE_STATIC_ENGINE "Use the static library version of the engine (off by default)" OFF)

# Set this to true in order to compile with asan enabled
set(ASAN_ON FALSE)

if(ASAN_ON)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    endif()
endif()

add_subdirectory(engine)
add_subdirectory(examples)
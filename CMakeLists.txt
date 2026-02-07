cmake_minimum_required(VERSION 4.2.0)
project(SDL-test LANGUAGES C CXX)

find_package(SDL3 REQUIRED)

# Right now the required external libraries are SDL3, shaderc, and glslang (this is a dependency of shaderc)
# Shaderc generally does not come with a package, so it is necessary to use the find_library and find_path function unfortunately
find_library(shaderc_lib NAMES shaderc_combined PATHS /usr/lib)
find_path(shaderc_path NAMES shaderc PATHS /usr/include)
find_package(glslang REQUIRED)

if (shaderc_lib)
    message(STATUS "Found shaderc library at: ${shaderc_lib}")
else()
    message(FATAL_ERROR "Could not find shaderc library")
endif()

if (shaderc_path)
    message(STATUS "Found shaderc include path at: ${shaderc_path}")
else()
    message(FATAL_ERROR "Could not find shaderc include path")
endif()

add_executable(main src/main.c src/GPUBuffers.c src/Strings.c src/Shader.c src/SDLDevice.c src/GraphicsPipeline.c)
target_include_directories(main PRIVATE "include" ${shaderc_path})
target_link_libraries(main PRIVATE
    stdc++
    m
    glslang::glslang
    SDL3::SDL3
    ${shaderc_lib}
)
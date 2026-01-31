cmake_minimum_required(VERSION 3.16)
project(SDL-test LANGUAGES C)

find_package(SDL3 REQUIRED)
find_package(glslang CONFIG REQUIRED)

add_executable(main src/main.c src/GPUBuffers.c src/Strings.c src/Shader.c src/SDLDevice.c)
target_include_directories(main PRIVATE "include")
target_link_libraries(main PRIVATE SDL3::SDL3 glslang::glslang glslang::glslang-default-resource-limits)
cmake_minimum_required(VERSION 4.1.0)
project(SDL-test LANGUAGES C CXX)

# This must come before including the other cmake files
add_executable(main src/main.c src/GPUBuffers.c src/Strings.c src/Shader.c src/SDLDevice.c src/GraphicsPipeline.c src/MeshObject.c src/Window.c)

if(UNIX)
    target_link_libraries(main PRIVATE
        c
        stdc++
        m
    )
endif()

include(cmake/SDL3.cmake)
include(cmake/VulkanComponents.cmake)
include(cmake/cglm.cmake)

target_include_directories(main PRIVATE "include")

# Enable for address sanitization
# target_compile_options(main PRIVATE -fsanitize=address)
# target_link_options(main PRIVATE -fsanitize=address)
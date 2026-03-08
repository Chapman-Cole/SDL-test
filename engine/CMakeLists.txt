cmake_minimum_required(VERSION 4.1.0)
project(engine LANGUAGES C CXX)

# >>>>>>>>>>>>>>>>>>>>>> IMPORTANT USAGE NOTES <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
# When using the static library engine_combined, you must link to all of its 
# dependencies, which right now are SDL3, shaderc_combined, and cglm. When 
# using the shared object/dll version this is done for you, and the only 
# requirement is that the shared object files are in the system path or in
# the same directory as the executable file at runtime
# ---------------------------------------------------------------------------

# >>>>>>>>>>>>>>>>>>>>>>> OBJECT LIB COMPILATION <<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# Compile the files once to an object library so the static and dynamic link versions can be built later on
add_library(EngineObjectLib OBJECT src/GPUBuffers.c src/Strings.c src/Shader.c src/SDLDevice.c src/GraphicsPipeline.c src/MeshObject.c src/Window.c ../external/SPIRV-Reflect/spirv_reflect.c)

# Provide the object library with the necessary include directories for compilation
target_include_directories(EngineObjectLib PRIVATE "include" ../external ../external/SPIRV-Headers)

# This must come before including the other cmake files
set(project_name "EngineObjectLib")

# This will provide headers for the object files compiled above, and also
# helps with tracking linking
include(../cmake/use_SDL3.cmake)
include(../cmake/use_VulkanComponents.cmake)
include(../cmake/use_cglm.cmake)

# Shared object libraries (dynamic link librareis) require position independent code
set_target_properties(EngineObjectLib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------



# >>>>>>>>>>>>>>>>>>>>>> STATIC LIBRARY COMPILATION <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# Build the static library version of the library
add_library(EngineStaticLib STATIC $<TARGET_OBJECTS:EngineObjectLib>)

# This must come before including the other cmake files
set(project_name "EngineStaticLib")

# Since this is a static library, it will just record the dependencies, but won't actually link to anything
include(../cmake/use_SDL3.cmake)
include(../cmake/use_VulkanComponents.cmake)
include(../cmake/use_cglm.cmake)

# ------------------------------------------------------------------------------



# >>>>>>>>>>>>>>>>>>>>>>> SHARED LIBRARY COMPILATION <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# Build the shared object (dynamic link) version of the library
add_library(EngineSharedLib SHARED $<TARGET_OBJECTS:EngineObjectLib>)

# On windows, in order to get the import library for the dll, I have to manually
# export all symbols since I don't do that in the source code
set_target_properties(EngineSharedLib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

# This must come before including the other cmake files
set(project_name "EngineSharedLib")

# Since this is a shared library, the actual linkage will occur here
include(../cmake/use_SDL3.cmake)
include(../cmake/use_VulkanComponents.cmake)
include(../cmake/use_cglm.cmake)

if(UNIX)
    target_link_libraries(${project_name} PRIVATE
        m
        stdc++
    )
endif()

# ------------------------------------------------------------------------------

# The static library will be called engine_combined, while the the shared library will just be called engine
# On windows, you will have engine_combined.lib and engine.lib in the lib folder, while engine.dll is placed in 
# in the bin folder. In this case, engine.lib is just an import library for engine.dll, while engine_combined.lib 
# is an actual static library containing all the necessary code
# On linux, you will have libengine_combined.a and libengine.so in the lib folder, making it a little more straightforward
set_target_properties(EngineStaticLib PROPERTIES OUTPUT_NAME "engine_combined")
set_target_properties(EngineSharedLib PROPERTIES OUTPUT_NAME "engine")


# Enable for address sanitization
# Linux Below
#target_compile_options(main PRIVATE -fsanitize=address -fno-omit-frame-pointer)
#target_link_options(main PRIVATE -fsanitize=address)

# Windows Below
#target_compile_options(${project_name} PRIVATE "/fsanitize=address" "/Zi")
#target_link_options(${project_name} PRIVATE "/fsanitize=address")
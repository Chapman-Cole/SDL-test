cmake_minimum_required(VERSION 4.1.0)
project(examples LANGUAGES C CXX)

# Depending on what the user sets this variable to, either the shared library or dynamic library will be used
# for the executable. By default, USE_STATIC_ENGINE is OFF, meaning the shared library version will be used
if(USE_STATIC_ENGINE)
    if(UNIX)
        set(library_name "libengine_combined.a")
    elseif(WIN32)
        set(library_name "engine_combined.lib")
    else()
        message(FATAL_ERROR "Unsupported platform for engine library")
    endif()
else()
    if(UNIX)
        set(library_name "libengine.so")
    elseif(WIN32)
        set(library_name "engine.lib")
    endif()
endif()

# >>>>>>>>>>>>>>>>>>> EXAMPLE 1 COMPILATION <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# This must come before including the other cmake files
set(project_name "example1")

add_executable(${project_name} example1.c)

# It is very important the internal engine library comes first
# so that way the linker can resolve its dependencies by looking
# in the linked libraries that come after
target_include_directories(${project_name} PRIVATE ../engine/include)

target_link_libraries(${project_name} PRIVATE ${CMAKE_SOURCE_DIR}/build/lib/${library_name})

include(../cmake/use_SDL3.cmake)
include(../cmake/use_VulkanComponents.cmake)
include(../cmake/use_cglm.cmake)

if(UNIX)
    target_link_libraries(${project_name} PRIVATE m stdc++)
endif()

# --------------------------------------------------------------------------------------


# >>>>>>>>>>>>>>>>>>>>>>> EXAMPLE 2 COMPILATION <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

set(project_name "example2")

add_executable(${project_name} example2.c)

# It is very important the internal engine library comes first
# so that way the linker can resolve its dependencies by looking
# in the linked libraries that come after
target_include_directories(${project_name} PRIVATE ../engine/include)

target_link_libraries(${project_name} PRIVATE ${CMAKE_SOURCE_DIR}/build/lib/${library_name})

include(../cmake/use_SDL3.cmake)
include(../cmake/use_VulkanComponents.cmake)
include(../cmake/use_cglm.cmake)

if(UNIX)
    target_link_libraries(${project_name} PRIVATE m stdc++)
endif()

# -------------------------------------------------------------------------------------




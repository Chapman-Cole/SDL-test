# The cmake module responsible for finding the SDL3_image libraries and packages

find_package(SDL3_image)

if(SDL3_image_FOUND)
    message(STATUS "Found SDL3_image package")
else()
    find_library(sdl3_image_lib NAMES SDL3_image PATHS /usr/lib)
    if (sdl3_image_lib)
        message(STATUS "SDL3_image lib found at ${sdl3_image_lib}")
    else()
        message(FATAL_ERROR "SDL3_image lib not found")
    endif()

    find_path(sdl3_image_path NAMES SDL3_image PATHS /usr/include)
    if (sdl3_image_path)
        message(STATUS "SDL3_image path found at ${sdl3_image_path}")
    else()
        message(FATAL_ERROR "SDL3_image path not found")
    endif()
endif()
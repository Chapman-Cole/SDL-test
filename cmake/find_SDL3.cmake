# The cmake module responsible for finding the SDL3 libraries and packages
# PUBLIC linking because the engine library also requires the use of SDL3
# PRIVATE can only be used if only the target needs the library

find_package(SDL3)

if(SDL3_FOUND)
    message(STATUS "Found SDL3 package")
else()
    find_library(sdl3_lib NAMES SDL3 PATHS /usr/lib C:/Users/chapmanc7563/Desktop/SDL3-3.4.0/lib/x64)
    if(sdl3_lib)
        message(STATUS "SDL3 lib found at ${sdl3_lib}")
    else()
        message(FATAL_ERROR "SDL3 lib not found")
    endif()

    find_path(sdl3_path NAMES SDL3 PATHS /usr/include C:/Users/chapmanc7563/Desktop/SDL3-3.4.0/include)
    if(sdl3_path)
        message(STATUS "SDL3 path found at ${sdl3_path}")
    else()
        message(FATAL_ERROR "SDL3 path not found")
    endif()
endif()
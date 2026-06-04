# The cmake module responsible for finding the SDL3_ttf libraries and packages

find_package(SDL3_ttf)

if(SDL3_ttf_FOUND)
    message(STATUS "Found SDL3_ttf package.")
else()
    find_library(sdl3_ttf_lib NAMES SDL3_ttf PATHS /usr/lib)
    if(sdl3_ttf_lib)
        message(STATUS "SDL3_ttf lib found at ${sdl3_ttf_lib} ")
    else()
        message(FATAL_ERROR "SDL3_ttf lib not found")
    endif()

    find_path(sdl3_ttf_path NAMES SDL3_ttf PATHS /usr/include)
    if(sdl3_ttf_path)
        message(STATUS "SDL3_ttf path found at ${sdl3_ttf_path}")
    else()
        message(FATAL_ERROR "SDL3_ttf path not found")
    endif()
endif()
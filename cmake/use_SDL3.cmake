# Assumes that find_SDL3.cmake has already been called to locate the necessary dependencies
# The project_name variable is assumed to be set by you manually
# PUBLIC linking because the engine library also requires the use of shaderc
# PRIVATE can only be used if only the target needs the library

if(SDL3_FOUND)
    target_link_libraries(${project_name} PUBLIC SDL3::SDL3)
else()
    target_link_libraries(${project_name} PUBLIC ${sdl3_lib})
    target_include_directories(${project_name} PUBLIC ${sdl3_path})
endif()
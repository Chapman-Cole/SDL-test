# Assumes that find_SDL3_ttf.cmake has already been called to locate the necessary dependencies
# The project_name variable is assumed to be set by you manually

if(SDL3_ttf_FOUND)
    target_link_libraries(${project_name} PUBLIC SDL3_ttf::SDL3_ttf)
else()
    target_link_libraries(${project_name} PUBLIC ${sdl3_ttf_lib})
    target_include_directories(${project_name} PUBLIC ${sdl3_ttf_path})
endif()
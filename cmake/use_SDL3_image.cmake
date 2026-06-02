# Assumes that find_SDL3_image.cmake has already been called to locate the necessary dependencies
# The project_name variable is assumed to be set by you manually

if(SDL3_image_FOUND)
    target_link_libraries(${project_name} PUBLIC SDL3_image::SDL3_image)
else()
    target_link_libraries(${project_name} PUBLIC ${sdl3_image_lib})
    target_include_directories(${project_name} PUBLIC ${sdl3_image_path})
endif()
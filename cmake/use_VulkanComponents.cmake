# Links to VulkanComponents packages and libraries (shaderc_combined and its dependencies)
# Assumes find_VulkanComponents.cmake has already been called to get the necessary locations
# The project_name variable is assumed to be set by you manually
# PUBLIC linking because the engine library also requires the use of shaderc
# PRIVATE can only be used if only the target needs the library

if(Vulkan_FOUND)
    if(UNIX)
        if(glslang_FOUND)
            target_link_libraries(${project_name} PUBLIC glslang::glslang)
        else()
            target_link_libraries(${project_name} PUBLIC ${glslang_lib})
            target_include_directories(${project_name} PUBLIC ${glslang_path})
        endif()
        target_link_libraries(${project_name} PUBLIC Vulkan::shaderc_combined)
    else()
        target_link_libraries(${project_name} PUBLIC Vulkan::glslang Vulkan::shaderc_combined)
    endif()
else()
    if(glslang_FOUND)
        target_link_libraries(${project_name} PUBLIC glslang::glslang)
    else()
        target_link_libraries(${project_name} PUBLIC ${glslang_lib})
        target_include_directories(${project_name} PUBLIC ${glslang_path})
    endif()

    target_link_libraries(${project_name} PUBLIC ${shaderc_lib})
    target_include_directories(${project_name} PUBLIC ${shaderc_path})
endif()
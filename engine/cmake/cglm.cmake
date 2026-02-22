# The cmake module responsible for locating the cglm headers

find_path(cglm_path NAMES cglm PATHS /usr/include include/)

if(cglm_path)
    message(STATUS "Found cglm path at ${cglm_path}")
    target_include_directories(${project_exe_name} PRIVATE ${cglm_path})
else()
    message(FATAL_ERROR "Could not find cglm path")
endif()
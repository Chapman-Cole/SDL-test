# Will add cglm headers to the include headers for the project with the name specified by
# the variable project_name, which you set. Assumes find_cglm.cmake has been called already for the path
target_include_directories(${project_name} PRIVATE ${cglm_path})
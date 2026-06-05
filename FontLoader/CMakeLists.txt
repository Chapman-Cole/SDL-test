cmake_minimum_required(VERSION 4.1.0)
project(FontLoader LANGUAGES C CXX)

# >>>>>>>>>>>>>>>>>>>>>>> OBJECT LIB COMPILATION <<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# Compile the files once to an object library so the static and dynamic link versions can be built later on
add_library(FontLoaderObjectLib OBJECT src/FontParser.c)

# Provide the object library with the necessary include directories for compilation
target_include_directories(FontLoaderObjectLib PRIVATE "include")

# This must come before including the other cmake files
# For now, no other libraries are needed
set(project_name "FontLoaderObjectLib")

# Shared object libraries (dynamic link librareis) require position independent code
set_target_properties(FontLoaderObjectLib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------



# >>>>>>>>>>>>>>>>>>>>>> STATIC LIBRARY COMPILATION <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# Build the static library version of the library
add_library(FontLoaderStaticLib STATIC $<TARGET_OBJECTS:FontLoaderObjectLib>)

# This must come before including the other cmake files
# For now, no other libraries are needed
set(project_name "FontLoaderStaticLib")

# ------------------------------------------------------------------------------



# >>>>>>>>>>>>>>>>>>>>>>> SHARED LIBRARY COMPILATION <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# Build the shared object (dynamic link) version of the library
add_library(FontLoaderSharedLib SHARED $<TARGET_OBJECTS:FontLoaderObjectLib>)

# On windows, in order to get the import library for the dll, I have to manually
# export all symbols since I don't do that in the source code
set_target_properties(FontLoaderSharedLib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

# This must come before including the other cmake files
# For now, no other libraries are needed
set(project_name "FontLoaderSharedLib")

# ------------------------------------------------------------------------------

set_target_properties(FontLoaderStaticLib PROPERTIES OUTPUT_NAME "fontloader_combined")
set_target_properties(FontLoaderSharedLib PROPERTIES OUTPUT_NAME "fontloader")
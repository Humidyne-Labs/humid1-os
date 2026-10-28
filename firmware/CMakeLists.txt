cmake_minimum_required(VERSION 3.20)

# Validate IDF_PATH environment variable
if(DEFINED ENV{IDF_PATH} AND NOT "$ENV{IDF_PATH}" STREQUAL "")
    set(IDF_PATH "$ENV{IDF_PATH}")
elseif(NOT DEFINED IDF_PATH OR IDF_PATH STREQUAL "")
    message(FATAL_ERROR "IDF_PATH is not set. Set it to your ESP-IDF installation directory before configuring this project.")
endif()

include("${IDF_PATH}/tools/cmake/project.cmake")
project(humid1_os)

# Suppress warnings in external managed dependencies (e.g., LVGL)
if(TARGET __idf_lvgl__lvgl)
    target_compile_options(__idf_lvgl__lvgl PRIVATE -w)
    set_target_properties(__idf_lvgl__lvgl PROPERTIES SYSTEM TRUE)
endif()

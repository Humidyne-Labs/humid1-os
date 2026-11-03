cmake_minimum_required(VERSION 3.16)

# Default to the 'dev' profile if SDKCONFIG_DEFAULTS is not explicitly provided
if(NOT DEFINED SDKCONFIG_DEFAULTS AND NOT DEFINED ENV{SDKCONFIG_DEFAULTS})
    set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.defaults.dev")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(humid1_os)

# Suppress false-positive -Werror=format-nonliteral on managed components under GCC 15+ (ESP-IDF v6.1)
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral" APPEND)

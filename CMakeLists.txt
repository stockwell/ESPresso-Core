cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_COMPILER xtensa-esp32-elf-gcc)
set(CMAKE_CXX_COMPILER xtensa-esp32-elf-g++)
set(CMAKE_ASM_COMPILER xtensa-esp32-elf-as)

set(IDF_TARGET esp32s3)

list(APPEND EXTRA_COMPONENT_DIRS
        src
        vendor/QuickPID
        vendor/ESP32-MAX31865
)

if (NOT DEFINED PROJECT_NAME)
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(ESPresso)
else ()
    message(FATAL_ERROR "ESPresso: This must be a project's main CMakeLists.txt.")
endif ()

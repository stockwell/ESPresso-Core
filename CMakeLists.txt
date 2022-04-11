cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND EXTRA_COMPONENT_DIRS
	src
	vendor/ESPresso-UI
	vendor/QuickPID
	vendor/lv_port_esp32/components/lvgl
	vendor/lv_port_esp32/components/lvgl_esp32_drivers
)

if (NOT DEFINED PROJECT_NAME)
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(ESPresso)
else()
    message(FATAL_ERROR "ESPresso: This must be a project's main CMakeLists.txt.")
endif()

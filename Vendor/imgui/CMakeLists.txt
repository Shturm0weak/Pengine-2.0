cmake_minimum_required(VERSION 3.8)

project(ImGui VERSION 1.0)

MESSAGE("Building ImGui")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP")

set(SOURCE_FILES
${PROJECT_SOURCE_DIR}/imgui.cpp ${PROJECT_SOURCE_DIR}/imgui.h
${PROJECT_SOURCE_DIR}/imgui_demo.cpp
${PROJECT_SOURCE_DIR}/imgui_draw.cpp
${PROJECT_SOURCE_DIR}/imgui_tables.cpp
${PROJECT_SOURCE_DIR}/imgui_widgets.cpp
${PROJECT_SOURCE_DIR}/backends/imgui_impl_glfw.cpp ${PROJECT_SOURCE_DIR}/backends/imgui_impl_glfw.h
${PROJECT_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp ${PROJECT_SOURCE_DIR}/backends/imgui_impl_vulkan.h)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor/imgui/backends)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Includes)

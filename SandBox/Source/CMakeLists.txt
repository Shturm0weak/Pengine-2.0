cmake_minimum_required(VERSION 3.8)

project(${APP_NAME} VERSION 1.0)

message("Building SandBox")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP")

set(CORE_SOURCES
	Core/ExampleApplication.cpp Core/ExampleApplication.h
	Core/main.cpp
)
source_group("Core" FILES ${CORE_SOURCES})

add_executable(${PROJECT_NAME} ${CORE_SOURCES})

target_compile_definitions(${PROJECT_NAME} PUBLIC PENGINE_ENGINE=0)

target_link_libraries(${PROJECT_NAME} PRIVATE Pengine)

target_include_directories(${PROJECT_NAME} PRIVATE ../../Pengine/Source)
target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Includes)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Includes/GLFW)

add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}")
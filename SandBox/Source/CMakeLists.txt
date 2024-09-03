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

if (MSVC)
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}")
endif ()

if (WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_BINARY_DIR}/Pengine/Build/$<CONFIG>
		${CMAKE_BINARY_DIR}/${APP_NAME}/Build/$<CONFIG>)
elseif (UNIX)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_BINARY_DIR}/Pengine/Build
		${CMAKE_BINARY_DIR}/${APP_NAME}/Build)
endif ()
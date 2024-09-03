cmake_minimum_required(VERSION 3.14)

project(Test VERSION 1.0)

message("Building Test")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP")

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(CORE_SOURCES
	Scene.cpp
)
source_group("Core" FILES ${CORE_SOURCES})

add_executable(${PROJECT_NAME} ${CORE_SOURCES})

target_compile_definitions(${PROJECT_NAME} PUBLIC PENGINE_ENGINE=0)

target_link_libraries(${PROJECT_NAME} PRIVATE Pengine)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest_main)

target_include_directories(${PROJECT_NAME} PRIVATE ../Pengine/Source)
target_include_directories(${PROJECT_NAME} PRIVATE ../Vendor)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

if (MSVC)
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}")
endif ()

if (WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
		COMMAND "${CMAKE_COMMAND}" -E copy_directory
		${CMAKE_BINARY_DIR}/bin/$<CONFIG>
		${CMAKE_BINARY_DIR}/${PROJECT_NAME}/Build/$<CONFIG>)
endif ()

if (WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_BINARY_DIR}/Pengine/Build/$<CONFIG>
		${CMAKE_BINARY_DIR}/${PROJECT_NAME}/Build/$<CONFIG>)
endif ()

if (WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
		COMMAND "${CMAKE_COMMAND}" -E copy_directory
		${CMAKE_BINARY_DIR}/Vendor/assimp/Build/bin/$<CONFIG>
		${CMAKE_BINARY_DIR}/${PROJECT_NAME}/Build/$<CONFIG>)
endif ()

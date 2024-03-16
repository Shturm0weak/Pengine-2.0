cmake_minimum_required(VERSION 3.8)

project(Pengine VERSION 1.0)

set(APP_NAME "SandBox")

if (WIN32)
	set(GLFW_PATH "C:/GLFW")
	set(GLFW_PATH_LIB ${GLFW_PATH}/Lib)
	set(GLFW_PATH_INCLUDE ${GLFW_PATH}/Includes)
endif ()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_subdirectory(Vendor/imgui Vendor/imgui/Build)
add_subdirectory(Vendor/yaml-cpp Vendor/yaml-cpp/Build)
add_subdirectory(Vendor/entt Vendor/entt/Build)
add_subdirectory(Vendor/assimp Vendor/assimp/Build)
add_subdirectory(Pengine/Source Pengine/Build)
add_subdirectory(SandBox/Source SandBox/Build)
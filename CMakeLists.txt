cmake_minimum_required(VERSION 3.8)

project(Pengine VERSION 1.0)

set(APP_NAME "SandBox")

add_subdirectory(Vendor/imgui Vendor/imgui/Build)
add_subdirectory(Vendor/yaml-cpp Vendor/yaml-cpp/Build)
add_subdirectory(Vendor/assimp Vendor/assimp/Build)
add_subdirectory(Pengine/Source Pengine/Build)
add_subdirectory(SandBox/Source SandBox/Build)
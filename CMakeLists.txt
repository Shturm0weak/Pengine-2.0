cmake_minimum_required(VERSION 3.8)

project(Pengine VERSION 1.0)

set(APP_NAME "SandBox")

set(GlfwPath "C:/GLFW")
set(GlfwLibPath ${GlfwPath}/Lib)
set(GlfwIncludePath ${GlfwPath}/Includes)
set(VulkanPath "C:/VulkanSDK/1.3.268.0")
set(VulkanLibPath ${VulkanPath}/Lib)
set(VulkanIncludePath ${VulkanPath}/Include)

add_subdirectory(Vendor/imgui Vendor/imgui/Build)
add_subdirectory(Vendor/yaml-cpp Vendor/yaml-cpp/Build)
add_subdirectory(Vendor/entt Vendor/entt/Build)
add_subdirectory(Vendor/assimp Vendor/assimp/Build)
add_subdirectory(Pengine/Source Pengine/Build)
add_subdirectory(SandBox/Source SandBox/Build)
cmake_minimum_required(VERSION 3.8)

project(Editor VERSION 1.0)

message("Building Editor")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP")

set(CORE_SOURCES
	Core/Editor.cpp Core/Editor.h
    Core/EditorApplication.cpp Core/EditorApplication.h
	Core/EntityAnimatorEditor.cpp Core/EntityAnimatorEditor.h
	Core/ImGuizmo.cpp Core/ImGuizmo.h
	Core/ThumbnailAtlas.cpp Core/ThumbnailAtlas.h
    Core/main.cpp
)
source_group("Core" FILES ${CORE_SOURCES})

add_executable(${PROJECT_NAME} ${CORE_SOURCES})

target_compile_definitions(${PROJECT_NAME} PUBLIC PENGINE_ENGINE=0)

target_link_libraries(${PROJECT_NAME} PRIVATE Jolt)
target_link_libraries(${PROJECT_NAME} PRIVATE YAML)
target_link_libraries(${PROJECT_NAME} PRIVATE ImGui)
target_link_libraries(${PROJECT_NAME} PRIVATE Pengine)

target_include_directories(${PROJECT_NAME} PRIVATE $ENV{GLFW_INCLUDE_PATH})
target_include_directories(${PROJECT_NAME} PRIVATE ../../Pengine/Source)
target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor/fastgltf/include)
target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor/JoltPhysics)
target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor)

if (MSVC)
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/${APP_NAME}")
endif ()

if (WIN32)
	if (MINGW)
		if (GENERATOR_IS_MULTI_CONFIG)
			add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				$ENV{GLFW_LIB_PATH}/glfw3.dll
				${CMAKE_BINARY_DIR}/${PROJECT_NAME}/Build/$<CONFIG>/glfw3.dll)

			add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				$ENV{VULKAN_BIN_PATH}/shaderc_shared.dll
				${CMAKE_BINARY_DIR}/${PROJECT_NAME}/Build/$<CONFIG>/shaderc_shared.dll)

			add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				$ENV{VULKAN_BIN_PATH}/shaderc_sharedd.dll
				${CMAKE_BINARY_DIR}/${PROJECT_NAME}/Build/$<CONFIG>/shaderc_sharedd.dll)
		else ()
			add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				$ENV{GLFW_LIB_PATH}/glfw3.dll
				${CMAKE_BINARY_DIR}/${PROJECT_NAME}/Build/glfw3.dll)

			add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				$ENV{VULKAN_BIN_PATH}/shaderc_shared.dll
				${CMAKE_BINARY_DIR}/${PROJECT_NAME}/Build/shaderc_shared.dll)

			add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				$ENV{VULKAN_BIN_PATH}/shaderc_sharedd.dll
				${CMAKE_BINARY_DIR}/${PROJECT_NAME}/Build/shaderc_sharedd.dll)
		endif ()
	endif()
endif ()

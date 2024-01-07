cmake_minimum_required(VERSION 3.8)

project(Pengine VERSION 1.0)

message("Building Pengine")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP")

set(COMPONENTS_SOURCES
	Components/Camera.cpp Components/Camera.h
	Components/PointLight.cpp Components/PointLight.h
	Components/Transform.cpp Components/Transform.h
	Components/Renderer3D.cpp Components/Renderer3D.h
)
source_group("Components" FILES ${COMPONENTS_SOURCES})

set(CORE_SOURCES
	Core/Asset.h
	Core/Application.cpp Core/Application.h
	Core/ColoredOutput.h
	Core/Core.h
	Core/Entity.cpp Core/Entity.h
	Core/EntryPoint.cpp Core/EntryPoint.h
	Core/FileFormatNames.h
	Core/Input.cpp Core/Input.h
	Core/KeyCode.h
	Core/Logger.cpp Core/Logger.h
	Core/MaterialManager.cpp Core/MaterialManager.h
	Core/MeshManager.cpp Core/MeshManager.h
	Core/ReflectionSystem.h
	Core/RenderPassManager.cpp Core/RenderPassManager.h
	Core/Scene.cpp Core/Scene.h
	Core/SceneManager.cpp Core/SceneManager.h
	Core/Serializer.cpp Core/Serializer.h
	Core/TextureManager.cpp Core/TextureManager.h
	Core/TextureSlots.cpp Core/TextureSlots.h
	Core/Time.cpp Core/Time.h
	Core/Timer.cpp Core/Timer.h
	Core/UUID.cpp Core/UUID.h
	Core/Viewport.cpp Core/Viewport.h
	Core/ViewportManager.cpp Core/ViewportManager.h
	Core/Window.cpp Core/Window.h
	Core/WindowManager.cpp Core/WindowManager.h
)
source_group("Core" FILES ${CORE_SOURCES})

set(EDITOR_SOURCES
	Editor/Editor.cpp Editor/Editor.h
	Editor/ImGuizmo.cpp Editor/ImGuizmo.h
)
source_group("Editor" FILES ${EDITOR_SOURCES})

#set(SOFTWARE_RENDERER_SOURCES
#	SoftwareRenderer/SrBuffer.cpp SoftwareRenderer/SrBuffer.h
#	SoftwareRenderer/SrTexture.cpp SoftwareRenderer/SrTexture.h
#	SoftwareRenderer/SrWindow.cpp SoftwareRenderer/SrWindow.h
#)
#source_group("SoftwareRenderer" FILES ${SOFTWARE_RENDERER_SOURCES})

set(GRAPHICS_SOURCES
	Graphics/BaseMaterial.cpp Graphics/BaseMaterial.h
	Graphics/Buffer.cpp Graphics/Buffer.h
	Graphics/FrameBuffer.cpp Graphics/FrameBuffer.h
	Graphics/Mesh.cpp Graphics/Mesh.h
	Graphics/Material.cpp Graphics/Material.h
	Graphics/Pipeline.cpp Graphics/Pipeline.h
	Graphics/Renderer.cpp Graphics/Renderer.h
	Graphics/RenderPass.cpp Graphics/RenderPass.h
	Graphics/stb_image.h
	Graphics/Texture.cpp Graphics/Texture.h
	Graphics/UniformLayout.cpp Graphics/UniformLayout.h
	Graphics/UniformWriter.cpp Graphics/UniformWriter.h
	Graphics/Vertex.cpp Graphics/Vertex.h
)
source_group("Graphics" FILES ${GRAPHICS_SOURCES})

set(VULKAN_SOURCES
	Vulkan/VulkanBuffer.cpp Vulkan/VulkanBuffer.h
	Vulkan/VulkanDescriptors.cpp Vulkan/VulkanDescriptors.h
	Vulkan/VulkanDevice.cpp Vulkan/VulkanDevice.h
	Vulkan/VulkanFrameBuffer.cpp Vulkan/VulkanFrameBuffer.h
	Vulkan/VulkanPipeline.cpp Vulkan/VulkanPipeline.h
	Vulkan/VulkanRenderer.cpp Vulkan/VulkanRenderer.h
	Vulkan/VulkanRenderPass.cpp Vulkan/VulkanRenderPass.h
	Vulkan/VulkanTexture.cpp Vulkan/VulkanTexture.h
	Vulkan/VulkanUniformLayout.cpp Vulkan/VulkanUniformLayout.h
	Vulkan/VulkanUniformWriter.cpp Vulkan/VulkanUniformWriter.h
	Vulkan/VulkanWindow.cpp Vulkan/VulkanWindow.h
)
source_group("Vulkan" FILES ${VULKAN_SOURCES})

set(UTILS_SOURCES
	Utils/Utils.cpp Utils/Utils.h
)
source_group("Utils" FILES ${UTILS_SOURCES})

set(CONFIGS_SOURCES
	Configs/EngineConfig.h
)
source_group("Configs" FILES ${CONFIGS_SOURCES})

set(EVENT_SYSTEM_SOURCES
	EventSystem/Event.h
	EventSystem/EventSystem.cpp EventSystem/EventSystem.h
	EventSystem/NextFrameEvent.h
	EventSystem/ResizeEvent.h
)
source_group("EventSystem" FILES ${EVENT_SYSTEM_SOURCES})

add_library(${PROJECT_NAME} SHARED
	${COMPONENTS_SOURCES}
	${CORE_SOURCES}
	#${SOFTWARE_RENDERER_SOURCES}
	${GRAPHICS_SOURCES}
	${VULKAN_SOURCES}
	${UTILS_SOURCES}
	${CONFIGS_SOURCES}
	${EVENT_SYSTEM_SOURCES}
	${EDITOR_SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_definitions(${PROJECT_NAME} PUBLIC PENGINE_ENGINE=1)

target_link_libraries(${PROJECT_NAME} PRIVATE YAML)
target_link_libraries(${PROJECT_NAME} PRIVATE ImGui)
target_link_libraries(${PROJECT_NAME} PRIVATE EnTT)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp)

IF (NOT GLFW_FOUND)
	find_library(GLFW_LIBRARY NAMES glfw3 HINTS ${CMAKE_SOURCE_DIR}/Libs REQUIRED)
	IF (GLFW_LIBRARY)
		set(GLFW_FOUND ON)
		message(${GLFW_LIBRARY} " has been found")
	ELSE()
		message(FATAL_ERROR "GLFW library has not been found")
	ENDIF()
ENDIF()

target_link_libraries(${PROJECT_NAME} PRIVATE ${GLFW_LIBRARY})

IF (NOT VULKAN_FOUND)
	find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS ${CMAKE_SOURCE_DIR}/Libs REQUIRED)
	IF (VULKAN_LIBRARY)
		set(VULKAN_FOUND ON)
		message(${VULKAN_LIBRARY} " has been found")
	ELSE()
		message(FATAL_ERROR "Vulkan library has not been found")
	ENDIF()
ENDIF()

target_link_libraries(${PROJECT_NAME} PRIVATE ${VULKAN_LIBRARY})

IF (NOT SHADERC_COMBINED_R_FOUND)
	find_library(SHADERC_COMBINED_R_LIBRARY NAMES shaderc_r HINTS ${CMAKE_SOURCE_DIR}/Libs REQUIRED)
	IF (SHADERC_COMBINED_R_LIBRARY)
		set(SHADERC_COMBINED_R_FOUND ON)
		message(${SHADERC_COMBINED_R_LIBRARY} " has been found")
	ELSE()
		message(FATAL_ERROR "Shaderc combined library release has not been found")
	ENDIF()
ENDIF()

IF (NOT SHADERC_COMBINED_D_FOUND)
	find_library(SHADERC_COMBINED_D_LIBRARY NAMES shaderc_d HINTS ${CMAKE_SOURCE_DIR}/Libs REQUIRED)
	IF (SHADERC_COMBINED_D_LIBRARY)
		set(SHADERC_COMBINED_D_FOUND ON)
		message(${SHADERC_COMBINED_D_LIBRARY} " has been found")
	ELSE()
		message(FATAL_ERROR "Shaderc combined library debug has not been found")
	ENDIF()
ENDIF()

target_link_libraries(${PROJECT_NAME} PRIVATE debug ${SHADERC_COMBINED_D_LIBRARY} optimized ${SHADERC_COMBINED_R_LIBRARY})

target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor)
target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor/ImGui)
target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor/assimp/include)
target_include_directories(${PROJECT_NAME} PRIVATE ../../Vendor/shaderc/libshaderc/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Includes)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Includes/GLFW)

add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
	${CMAKE_BINARY_DIR}/${APP_NAME}/Build/$<CONFIG>)
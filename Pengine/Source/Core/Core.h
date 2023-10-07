#pragma once

#pragma warning(disable : 4715)
#pragma warning(disable : 4251)
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)
#pragma warning(disable : 26812)
#pragma warning(disable : 26451)
#pragma warning(disable : 26495)
#define _CRT_OBSOLETE_NO_WARNINGS

#ifdef PENGINE_ENGINE
#define PENGINE_API __declspec(dllexport)
#else
#define PENGINE_API __declspec(dllimport)
#endif

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "UUID.h"

#define MAX_MATERIALS 500
#define MAX_TEXTURES 32

inline constexpr char* none = "None";
inline constexpr char* plane = "Plane";

template<typename T>
std::string GetTypeName()
{
	std::string typeName = typeid(T).name();
	if (typeName.find(std::string("class")) == 0)
	{
		return typeName.substr(sizeof("class"));
	}
	else if (typeName.find(std::string("struct")) == 0)
	{
		return typeName.substr(sizeof("struct"));
	}
	else
	{
		return typeName;
	}
}

enum class GraphicsAPI
{
	Software,
	Ogl,
	Vk
};

inline GraphicsAPI graphicsAPI;

namespace Pengine
{
	inline std::unordered_map<std::string, std::string> filepathByUuid;
	inline std::unordered_map<std::string, std::string> uuidByFilepath;
	inline int drawCallsCount;
	inline size_t vertexCount;

	namespace Vk
	{
		class VulkanDevice;
		class VulkanDescriptorPool;
		inline std::shared_ptr<VulkanDevice> device = nullptr;
		inline std::shared_ptr<VulkanDescriptorPool> descriptorPool = nullptr;
		inline uint32_t swapChainImageCount = 0;
		inline uint32_t swapChainImageIndex = 0;
	}
}
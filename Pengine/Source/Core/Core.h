#pragma once

#pragma warning(disable : 4715)
#pragma warning(disable : 4251)
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)
#pragma warning(disable : 26812)
#pragma warning(disable : 26451)
#pragma warning(disable : 26495)
#define CRT_OBSOLETE_NO_WARNINGS

#ifdef _WIN32
	#ifdef PENGINE_ENGINE
		#define PENGINE_API __declspec(dllexport)
	#else
		#define PENGINE_API __declspec(dllimport)
	#endif
#elif __linux__
	#ifdef PENGINE_ENGINE
		#define PENGINE_API __attribute__((visibility("default")))
	#else
		#define PENGINE_API
	#endif
#endif
#include <algorithm>
#include <array>
#include <functional>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

#include <entt/src/entt/entt.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "UUID.h"

inline constexpr char const* none = "None";
inline constexpr char const* plane = "Plane";

inline constexpr glm::vec3 topLevelRenderPassDebugColor = { 0.5f, 1.0f, 0.5f };

struct path_hash
{
	std::size_t operator()(const std::filesystem::path& path) const
	{
		return hash_value(path);
	}
};

template<typename T>
std::string GetTypeName()
{
	if (const std::string typeName = typeid(T).name(); typeName.find(std::string("class")) == 0)
	{
		return typeName.substr(sizeof("class"));
	}
	else if (typeName.find(std::string("struct")) == 0)
	{
		return typeName.substr(sizeof("struct"));
	}

	return typeid(T).name();
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
	inline std::unordered_map<std::string, std::filesystem::path> filepathByUuid;
	inline std::unordered_map<std::filesystem::path, std::string, path_hash> uuidByFilepath;

	// TODO: Maybe move this somewhere!
	inline int drawCallsCount;
	inline size_t vertexCount;
	inline size_t currentFrame = 0;

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

#undef NO_EDITOR
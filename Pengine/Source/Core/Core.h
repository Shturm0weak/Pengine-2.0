#pragma once

#pragma warning(disable : 4715)
#pragma warning(disable : 4251)
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)
#pragma warning(disable : 26812)
#pragma warning(disable : 26451)
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
#include <atomic>
#include <condition_variable>
#include <functional>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
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
inline constexpr const char* GetTypeName()
{
	return entt::type_name<std::remove_cv_t<std::remove_reference_t<T>>>().value().data();
}

template<typename T>
inline constexpr entt::id_type GetTypeHash()
{
	return entt::type_hash<std::remove_cv_t<std::remove_reference_t<T>>>().value();
}

enum class MemoryType
{
	CPU,
	GPU
};

enum class GraphicsAPI
{
	Software,
	Ogl,
	Vk
};

inline GraphicsAPI graphicsAPI;

namespace Pengine
{
	inline std::mutex uuidMutex;
	inline std::unordered_map<UUID, std::filesystem::path, uuid_hash> filepathByUuid;
	inline std::unordered_map<std::filesystem::path, UUID, path_hash> uuidByFilepath;

	// TODO: Maybe move this somewhere!
	inline int drawCallsCount;
	inline size_t vertexCount;
	inline size_t currentFrame = 0;
	inline int64_t vramAllocated = 0;

	inline std::shared_ptr<class Device> device = nullptr;

	namespace Vk
	{
		inline uint32_t swapChainImageCount = 0;
		inline uint32_t swapChainImageIndex = 0;
	}

}

#undef NO_EDITOR
#pragma once

#include "Core.h"

#include <future>
#include <mutex>

namespace Pengine
{

	class PENGINE_API AsyncAssetLoader
	{
	public:
		static AsyncAssetLoader& GetInstance();

		AsyncAssetLoader(const AsyncAssetLoader&) = delete;
		AsyncAssetLoader& operator=(const AsyncAssetLoader&) = delete;

		void AsyncLoadMaterial(const std::filesystem::path& filepath, std::function<void(std::shared_ptr<class Material>)>&& callback);

		void AsyncLoadBaseMaterial(const std::filesystem::path& filepath, std::function<void(std::shared_ptr<class BaseMaterial>)>&& callback);

		void AsyncLoadMesh(const std::filesystem::path& filepath, std::function<void(std::shared_ptr<class Mesh>)>&& callback);

		std::shared_ptr<class Material> SyncLoadMaterial(const std::filesystem::path& filepath);

		std::shared_ptr<class BaseMaterial> SyncLoadBaseMaterial(const std::filesystem::path& filepath);

		std::shared_ptr<class Mesh> SyncLoadMesh(const std::filesystem::path& filepath);

		void Update();

	private:
		AsyncAssetLoader() = default;
		~AsyncAssetLoader() = default;

		std::unordered_map<std::filesystem::path, std::vector<std::function<void(std::shared_ptr<class Material>)>>> m_MaterialsToBeLoaded;
		std::unordered_map<std::filesystem::path, std::vector<std::function<void(std::shared_ptr<class BaseMaterial>)>>> m_BaseMaterialsToBeLoaded;
		std::unordered_map<std::filesystem::path, std::vector<std::function<void(std::shared_ptr<class Mesh>)>>> m_MeshesToBeLoaded;

		std::mutex m_MaterialMutex;
		std::mutex m_BaseMaterialMutex;
		std::mutex m_MeshMutex;

		std::unordered_set<std::filesystem::path> m_MaterialsLoading;
		std::unordered_set<std::filesystem::path> m_BaseMaterialsLoading;
		std::unordered_set<std::filesystem::path> m_MeshesLoading;
	};

}

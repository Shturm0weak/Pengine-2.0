#include "AsyncAssetLoader.h"

#include "ThreadPool.h"
#include "MaterialManager.h"
#include "MeshManager.h"

using namespace Pengine;

AsyncAssetLoader& Pengine::AsyncAssetLoader::GetInstance()
{
	static AsyncAssetLoader asyncAssetLoader;
	return asyncAssetLoader;
}

void AsyncAssetLoader::AsyncLoadMaterial(const std::filesystem::path& filepath, std::function<void(std::shared_ptr<Material>)>&& callback)
{
	if (!m_MaterialsLoading.count(filepath))
	{
		ThreadPool::GetInstance().EnqueueAsync([this, filepath]()
		{
			MaterialManager::GetInstance().LoadMaterial(filepath);

			std::lock_guard lock(m_MaterialMutex);
			m_MaterialsLoading.erase(filepath);
		});
	}
	
	std::lock_guard lock(m_MaterialMutex);
	m_MaterialsLoading.emplace(filepath);
	m_MaterialsToBeLoaded[filepath].emplace_back(std::move(callback));
}

void AsyncAssetLoader::AsyncLoadBaseMaterial(const std::filesystem::path& filepath, std::function<void(std::shared_ptr<BaseMaterial>)>&& callback)
{
	if (!m_BaseMaterialsLoading.count(filepath))
	{
		ThreadPool::GetInstance().EnqueueAsync([this, filepath]()
		{
			MaterialManager::GetInstance().LoadBaseMaterial(filepath);

			std::lock_guard lock(m_BaseMaterialMutex);
			m_BaseMaterialsLoading.erase(filepath);
		});
	}

	std::lock_guard lock(m_BaseMaterialMutex);
	m_BaseMaterialsLoading.emplace(filepath);
	m_BaseMaterialsToBeLoaded[filepath].emplace_back(std::move(callback));
}

void AsyncAssetLoader::AsyncLoadMesh(const std::filesystem::path& filepath, std::function<void(std::shared_ptr<Mesh>)>&& callback)
{
	if (!m_MeshesLoading.count(filepath))
	{
		ThreadPool::GetInstance().EnqueueAsync([this, filepath]()
		{
			MeshManager::GetInstance().LoadMesh(filepath);

			std::lock_guard lock(m_MeshMutex);
			m_MeshesLoading.erase(filepath);
		});
	}

	std::lock_guard lock(m_MeshMutex);
	m_MeshesLoading.emplace(filepath);
	m_MeshesToBeLoaded[filepath].emplace_back(std::move(callback));
}

std::shared_ptr<Material> AsyncAssetLoader::SyncLoadMaterial(const std::filesystem::path& filepath)
{
	bool found = false;
	{
		std::lock_guard lock(m_MaterialMutex);
		found = (bool)m_MaterialsLoading.count(filepath);

		if (!found)
		{
			m_MaterialsLoading.emplace(filepath);
		}
	}

	if (found)
	{
		while (true)
		{
			std::shared_ptr<Material> material = MaterialManager::GetInstance().GetMaterial(filepath);
			if (material)
			{
				return material;
			}
		}
	}

	{
		std::lock_guard lock(m_MaterialMutex);
		m_MaterialsLoading.emplace(filepath);
	}

	return ThreadPool::GetInstance().EnqueueSync([this, filepath]()
	{
		std::shared_ptr<Material> material = MaterialManager::GetInstance().LoadMaterial(filepath);
		
		std::lock_guard lock(m_MaterialMutex);
		m_MaterialsLoading.erase(filepath);
		
		return material;
	}).get();
}

std::shared_ptr<BaseMaterial> AsyncAssetLoader::SyncLoadBaseMaterial(const std::filesystem::path& filepath)
{
	bool found = false;
	{
		std::lock_guard lock(m_BaseMaterialMutex);
		found = (bool)m_BaseMaterialsLoading.count(filepath);

		if (!found)
		{
			m_BaseMaterialsLoading.emplace(filepath);
		}
	}

	if (found)
	{
		while (true)
		{
			std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().GetBaseMaterial(filepath);
			if (baseMaterial)
			{
				return baseMaterial;
			}
		}
	}

	return ThreadPool::GetInstance().EnqueueSync([this, filepath]() mutable
	{
		std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(filepath);

		std::lock_guard lock(m_BaseMaterialMutex);
		m_BaseMaterialsLoading.erase(filepath);

		return baseMaterial;
	}).get();
}

std::shared_ptr<Mesh> AsyncAssetLoader::SyncLoadMesh(const std::filesystem::path& filepath)
{
	bool found = false;
	{
		std::lock_guard lock(m_MeshMutex);
		found = (bool)m_MeshesLoading.count(filepath);

		if (!found)
		{
			m_MeshesLoading.emplace(filepath);
		}
	}

	if (found)
	{
		while (true)
		{
			std::shared_ptr<Mesh> mesh = MeshManager::GetInstance().GetMesh(filepath);
			if (mesh)
			{
				return mesh;
			}
		}
	}

	{
		std::lock_guard lock(m_MeshMutex);
		m_MeshesLoading.emplace(filepath);
	}

	return ThreadPool::GetInstance().EnqueueSync([this, filepath]()
	{
		std::shared_ptr<Mesh> mesh = MeshManager::GetInstance().LoadMesh(filepath);

		std::lock_guard lock(m_MeshMutex);
		m_MeshesLoading.erase(filepath);

		return mesh;
	}).get();
}

void AsyncAssetLoader::Update()
{
	MaterialManager& materialManager = MaterialManager::GetInstance();

	for (auto& [filepath, callbacks] : m_MaterialsToBeLoaded)
	{
		const std::shared_ptr<Material> material = materialManager.GetMaterial(filepath);
		if (!material)
		{
			continue;
		}

		for (const auto& callback : callbacks)
		{
			callback(material);
		}

		std::lock_guard lock(m_MaterialMutex);
		m_MaterialsToBeLoaded.erase(filepath);
		break;
	}

	for (auto& [filepath, callbacks] : m_BaseMaterialsToBeLoaded)
	{
		const std::shared_ptr<BaseMaterial> baseMaterial = materialManager.GetBaseMaterial(filepath);
		if (!baseMaterial)
		{
			continue;
		}

		for (const auto& callback : callbacks)
		{
			callback(baseMaterial);
		}

		std::lock_guard lock(m_BaseMaterialMutex);
		m_BaseMaterialsToBeLoaded.erase(filepath);
		break;
	}

	for (auto& [filepath, callbacks] : m_MeshesToBeLoaded)
	{
		const std::shared_ptr<Mesh> mesh = MeshManager::GetInstance().GetMesh(filepath);
		if (!mesh)
		{
			continue;
		}

		for (const auto& callback : callbacks)
		{
			callback(mesh);
		}

		std::lock_guard lock(m_MeshMutex);
		m_MeshesToBeLoaded.erase(filepath);
		break;
	}
}
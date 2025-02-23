#include "AsyncAssetLoader.h"

#include "ThreadPool.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "TextureManager.h"

using namespace Pengine;

AsyncAssetLoader& Pengine::AsyncAssetLoader::GetInstance()
{
	static AsyncAssetLoader asyncAssetLoader;
	return asyncAssetLoader;
}

void AsyncAssetLoader::AsyncLoadMaterial(const std::filesystem::path& filepath, std::function<void(std::shared_ptr<Material>)>&& callback)
{
	std::lock_guard<std::mutex> lock(m_MaterialMutex);
	if (!m_MaterialsLoading.count(filepath))
	{
		ThreadPool::GetInstance().EnqueueAsync([this, filepath]()
		{
			MaterialManager::GetInstance().LoadMaterial(filepath);

			std::lock_guard<std::mutex> lock(m_MaterialMutex);
			m_MaterialsLoading.erase(filepath);
		});
	}

	m_MaterialsLoading.emplace(filepath);
	m_MaterialsToBeLoaded[filepath].emplace_back(std::move(callback));
}

void AsyncAssetLoader::AsyncLoadBaseMaterial(const std::filesystem::path& filepath, std::function<void(std::shared_ptr<BaseMaterial>)>&& callback)
{
	std::lock_guard<std::mutex> lock(m_BaseMaterialMutex);
	if (!m_BaseMaterialsLoading.count(filepath))
	{
		ThreadPool::GetInstance().EnqueueAsync([this, filepath]()
		{
			MaterialManager::GetInstance().LoadBaseMaterial(filepath);

			std::lock_guard<std::mutex> lock(m_BaseMaterialMutex);
			m_BaseMaterialsLoading.erase(filepath);
		});
	}

	m_BaseMaterialsLoading.emplace(filepath);
	m_BaseMaterialsToBeLoaded[filepath].emplace_back(std::move(callback));
}

void AsyncAssetLoader::AsyncLoadMesh(const std::filesystem::path& filepath, std::function<void(std::shared_ptr<Mesh>)>&& callback)
{
	std::lock_guard<std::mutex> lock(m_MeshMutex);
	if (!m_MeshesLoading.count(filepath))
	{
		ThreadPool::GetInstance().EnqueueAsync([this, filepath]()
		{
			MeshManager::GetInstance().LoadMesh(filepath);

			std::lock_guard<std::mutex> lock(m_MeshMutex);
			m_MeshesLoading.erase(filepath);
		});
	}

	m_MeshesLoading.emplace(filepath);
	m_MeshesToBeLoaded[filepath].emplace_back(std::move(callback));
}

void AsyncAssetLoader::AsyncLoadTexture(const std::filesystem::path& filepath, std::function<void(std::shared_ptr<class Texture>)>&& callback)
{
	std::lock_guard<std::mutex> lock(m_TextureMutex);
	if (!m_TexturesLoading.count(filepath))
	{
		ThreadPool::GetInstance().EnqueueAsync([this, filepath]()
		{
			TextureManager::GetInstance().Load(filepath);

			std::lock_guard<std::mutex> lock(m_TextureMutex);
			m_TexturesLoading.erase(filepath);
		});
	}

	m_TexturesLoading.emplace(filepath);
	m_TexturesToBeLoaded[filepath].emplace_back(std::move(callback));
}

std::shared_ptr<Material> AsyncAssetLoader::SyncLoadMaterial(const std::filesystem::path& filepath)
{
	bool found = false;
	{
		std::lock_guard<std::mutex> lock(m_MaterialMutex);
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
		std::lock_guard<std::mutex> lock(m_MaterialMutex);
		m_MaterialsLoading.emplace(filepath);
	}

	return ThreadPool::GetInstance().EnqueueSync([this, filepath]()
	{
		std::shared_ptr<Material> material = MaterialManager::GetInstance().LoadMaterial(filepath);
		
		std::lock_guard<std::mutex> lock(m_MaterialMutex);
		m_MaterialsLoading.erase(filepath);
		
		return material;
	}).get();
}

std::shared_ptr<BaseMaterial> AsyncAssetLoader::SyncLoadBaseMaterial(const std::filesystem::path& filepath)
{
	bool found = false;
	{
		std::lock_guard<std::mutex> lock(m_BaseMaterialMutex);
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

		std::lock_guard<std::mutex> lock(m_BaseMaterialMutex);
		m_BaseMaterialsLoading.erase(filepath);

		return baseMaterial;
	}).get();
}

std::shared_ptr<Mesh> AsyncAssetLoader::SyncLoadMesh(const std::filesystem::path& filepath)
{
	bool found = false;
	{
		std::lock_guard<std::mutex> lock(m_MeshMutex);
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
		std::lock_guard<std::mutex> lock(m_MeshMutex);
		m_MeshesLoading.emplace(filepath);
	}

	return ThreadPool::GetInstance().EnqueueSync([this, filepath]()
	{
		std::shared_ptr<Mesh> mesh = MeshManager::GetInstance().LoadMesh(filepath);

		std::lock_guard<std::mutex> lock(m_MeshMutex);
		m_MeshesLoading.erase(filepath);

		return mesh;
	}).get();
}

std::shared_ptr<Texture> AsyncAssetLoader::SyncLoadTexture(const std::filesystem::path& filepath)
{
	bool found = false;
	{
		std::lock_guard<std::mutex> lock(m_TextureMutex);
		found = (bool)m_TexturesLoading.count(filepath);

		if (!found)
		{
			m_TexturesLoading.emplace(filepath);
		}
	}

	if (found)
	{
		while (true)
		{
			std::shared_ptr<Texture> texture = TextureManager::GetInstance().GetTexture(filepath);
			if (texture)
			{
				return texture;
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_TextureMutex);
		m_TexturesLoading.emplace(filepath);
	}

	return ThreadPool::GetInstance().EnqueueSync([this, filepath]()
	{
		std::shared_ptr<Texture> texture = TextureManager::GetInstance().Load(filepath);

		std::lock_guard<std::mutex> lock(m_TextureMutex);
		m_TexturesLoading.erase(filepath);

		return texture;
	}).get();
}

void AsyncAssetLoader::Update()
{
	MaterialManager& materialManager = MaterialManager::GetInstance();

	{
		std::lock_guard<std::mutex> lock(m_MaterialMutex);
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

			m_MaterialsToBeLoaded.erase(filepath);
			break;
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_BaseMaterialMutex);
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

			m_BaseMaterialsToBeLoaded.erase(filepath);
			break;
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_MeshMutex);
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

			m_MeshesToBeLoaded.erase(filepath);
			break;
		}
	}
}

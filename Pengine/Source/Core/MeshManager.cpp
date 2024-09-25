#include "MeshManager.h"

#include "TextureManager.h"
#include "MaterialManager.h"
#include "Logger.h"
#include "Serializer.h"

using namespace Pengine;

MeshManager& MeshManager::GetInstance()
{
	static MeshManager meshManager;
	return meshManager;
}

std::shared_ptr<Mesh> MeshManager::CreateMesh(
	const std::string& name,
	const std::filesystem::path& filepath,
	std::vector<float>& vertices,
	std::vector<uint32_t>& indices)
{
	if (std::shared_ptr<Mesh> mesh = GetMesh(filepath))
	{
		return mesh;
	}
	else
	{
		mesh = std::make_shared<Mesh>(name, filepath, vertices, indices);
		std::lock_guard lock(m_MutexMesh);
		m_MeshesByFilepath[filepath] = mesh;

		return mesh;
	}
}

std::shared_ptr<Mesh> MeshManager::LoadMesh(const std::filesystem::path& filepath)
{
	if (std::shared_ptr<Mesh> mesh = GetMesh(filepath))
	{
		return mesh;
	}
	else
	{
		mesh = Serializer::DeserializeMesh(filepath);
		if (!mesh)
		{
			FATAL_ERROR(filepath.string() + ":There is no such mesh!");
		}

		std::lock_guard lock(m_MutexMesh);
		m_MeshesByFilepath.emplace(filepath, mesh);

		return mesh;
	}
}

std::shared_ptr<Mesh> MeshManager::GetMesh(const std::filesystem::path& filepath)
{
	auto meshByFilepath = m_MeshesByFilepath.find(filepath);
	if (meshByFilepath != m_MeshesByFilepath.end())
	{
		return meshByFilepath->second;
	}

	return nullptr;
}

void MeshManager::ShutDown()
{
	m_MeshesByFilepath.clear();
}

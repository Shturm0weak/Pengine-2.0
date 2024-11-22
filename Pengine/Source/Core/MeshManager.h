#pragma once

#include "Core.h"

#include "../Graphics/Mesh.h"

#include <mutex>

namespace Pengine
{

	class PENGINE_API MeshManager
	{
	public:
		static MeshManager& GetInstance();

		MeshManager(const MeshManager&) = delete;
		MeshManager& operator=(const MeshManager&) = delete;

		std::shared_ptr<Mesh> CreateMesh(
			const std::string& name,
			const std::filesystem::path& filepath,
			const size_t vertexSize,
			std::vector<float>& vertices,
			std::vector<uint32_t>& indices);

		std::shared_ptr<Mesh> LoadMesh(const std::filesystem::path& filepath);

		std::shared_ptr<Mesh> GetMesh(const std::filesystem::path& filepath);

		const std::unordered_map<std::filesystem::path, std::shared_ptr<Mesh>, path_hash>& GetMeshes() const { return m_MeshesByFilepath; }

		void ShutDown();

	private:
		MeshManager() = default;
		~MeshManager() = default;

		std::unordered_map<std::filesystem::path, std::shared_ptr<Mesh>, path_hash> m_MeshesByFilepath;

		std::mutex m_MutexMesh;
	};

}
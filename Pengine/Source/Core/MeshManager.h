#pragma once

#include "Core.h"

#include "../Graphics/Mesh.h"

namespace Pengine
{

	class PENGINE_API MeshManager
	{
	public:
		static MeshManager& GetInstance();

		MeshManager(const MeshManager&) = delete;
		MeshManager& operator=(const MeshManager&) = delete;

		std::shared_ptr<Mesh> CreateMesh(const std::string& name, const std::string& filepath,
			std::vector<float>& vertices, std::vector<uint32_t>& indices);

		std::shared_ptr<Mesh> LoadMesh(const std::string& filepath);

		std::shared_ptr<Mesh> GetMesh(const std::string& filepath);

		const std::unordered_map<std::string, std::shared_ptr<Mesh>>& GetMeshes() const { return m_MeshesByFilepath; }

		void ShutDown();

	private:
		MeshManager() = default;
		~MeshManager() = default;

		std::unordered_map<std::string, std::shared_ptr<Mesh>> m_MeshesByFilepath;
	};

}
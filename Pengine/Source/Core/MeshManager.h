#pragma once

#include "Core.h"

#include "../Graphics/Mesh.h"

namespace Pengine
{

	class PENGINE_API MeshManager
	{
	public:
		static MeshManager& GetInstance();

		std::shared_ptr<Mesh> LoadMesh(const std::string& filepath);

		std::shared_ptr<Mesh> GetMesh(const std::string& filepath);

		std::vector<std::shared_ptr<Mesh>> GenerateMeshes(const std::string& filepath);

		std::vector<std::shared_ptr<Mesh>> LoadIntermediate(const std::string& filepath);

		void ShutDown();

	private:
		MeshManager() = default;
		~MeshManager() = default;
		MeshManager(const MeshManager&) = delete;
		MeshManager& operator=(const MeshManager&) = delete;

		std::unordered_map<std::string, std::shared_ptr<Mesh>> m_MeshesByFilepath;
	};

}
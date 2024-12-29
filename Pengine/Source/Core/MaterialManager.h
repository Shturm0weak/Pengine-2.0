#pragma once

#include "Core.h"

#include "../Graphics/Material.h"

#include <mutex>

namespace Pengine
{

	class PENGINE_API MaterialManager
	{
	public:
		static MaterialManager& GetInstance();

		MaterialManager(const MaterialManager&) = delete;
		MaterialManager& operator=(const MaterialManager&) = delete;

		std::shared_ptr<Material> LoadMaterial(const std::filesystem::path& filepath);

		std::shared_ptr<BaseMaterial> LoadBaseMaterial(const std::filesystem::path& filepath);

		std::shared_ptr<Material> GetMaterial(const std::filesystem::path& filepath);

		std::shared_ptr<BaseMaterial> GetBaseMaterial(const std::filesystem::path& filepath);

		std::shared_ptr<Material> Clone(
			const std::string& name,
			const std::filesystem::path& filepath,
			const std::shared_ptr<Material>& material);

		const std::unordered_map<std::filesystem::path, std::shared_ptr<Material>, path_hash>& GetMaterials() const { return m_MaterialsByFilepath; }
		
		const std::unordered_map<std::filesystem::path, std::shared_ptr<BaseMaterial>, path_hash>& GetBaseMaterials() const { return m_BaseMaterialsByFilepath; }

		void DeleteMaterial(std::shared_ptr<Material> material);

		void DeleteBaseMaterial(std::shared_ptr<BaseMaterial> baseMaterial);

		void ReloadAll();

		void SaveAll();

		void ShutDown();

	private:
		MaterialManager() = default;
		~MaterialManager() = default;

		void ManipulateOnAllMaterialsDebug();

		std::unordered_map<std::filesystem::path, std::shared_ptr<Material>, path_hash> m_MaterialsByFilepath;
		std::unordered_map<std::filesystem::path, std::shared_ptr<BaseMaterial>, path_hash> m_BaseMaterialsByFilepath;

		std::mutex m_MutexMaterial;
		std::mutex m_MutexBaseMaterial;
	};

}
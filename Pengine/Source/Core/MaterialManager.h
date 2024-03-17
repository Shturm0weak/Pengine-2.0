#pragma once

#include "Core.h"

#include "../Graphics/Material.h"

namespace Pengine
{

	class PENGINE_API MaterialManager
	{
	public:
		static MaterialManager& GetInstance();

		MaterialManager(const MaterialManager&) = delete;
		MaterialManager& operator=(const MaterialManager&) = delete;

		std::shared_ptr<Material> LoadMaterial(const std::string& filepath);

		std::shared_ptr<BaseMaterial> LoadBaseMaterial(const std::string& filepath);

		std::shared_ptr<Material> GetMaterial(const std::string& filepath);

		std::shared_ptr<BaseMaterial> GetBaseMaterial(const std::string& filepath);

		std::shared_ptr<Material> Inherit(const std::string& name, const std::string& filepath,
			const std::shared_ptr<BaseMaterial>& baseMaterial);

		std::shared_ptr<Material> Clone(const std::string& name, const std::string& filepath,
			const std::shared_ptr<Material>& material);

		const std::unordered_map<std::string, std::shared_ptr<Material>>& GetMaterials() const { return m_MaterialsByFilepath; }
		
		const std::unordered_map<std::string, std::shared_ptr<BaseMaterial>>& GetBaseMaterials() const { return m_BaseMaterialsByFilepath; }

		void ShutDown();

	private:
		MaterialManager() = default;
		~MaterialManager() = default;

		std::unordered_map<std::string, std::shared_ptr<Material>> m_MaterialsByFilepath;
		std::unordered_map<std::string, std::shared_ptr<BaseMaterial>> m_BaseMaterialsByFilepath;
	};

}
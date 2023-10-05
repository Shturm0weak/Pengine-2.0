#include "MaterialManager.h"

#include "Logger.h"

using namespace Pengine;

MaterialManager& MaterialManager::GetInstance()
{
	static MaterialManager materialManager;
	return materialManager;
}

std::shared_ptr<Material> MaterialManager::LoadMaterial(const std::string& filepath)
{
	if (std::shared_ptr<Material> material = GetMaterial(filepath))
	{
		return material;
	}
	else
	{
		material = Material::Load(filepath);
		if (!material)
		{
			FATAL_ERROR(filepath + ":There is no such material!")
		}

		m_MaterialsByFilepath.emplace(filepath, material);

		return material;
	}
}

std::shared_ptr<BaseMaterial> MaterialManager::LoadBaseMaterial(const std::string& filepath)
{
	if (std::shared_ptr<BaseMaterial> baseMaterial = GetBaseMaterial(filepath))
	{
		return baseMaterial;
	}
	else
	{
		baseMaterial = BaseMaterial::Load(filepath);
		if (!baseMaterial)
		{
			FATAL_ERROR(filepath + ":There is no such base material!")
		}

		m_BaseMaterialsByFilepath.emplace(filepath, baseMaterial);

		return baseMaterial;
	}
}

std::shared_ptr<Material> MaterialManager::GetMaterial(const std::string& filepath)
{
	auto materialByFilepath = m_MaterialsByFilepath.find(filepath);
	if (materialByFilepath != m_MaterialsByFilepath.end())
	{
		return materialByFilepath->second;
	}

	return nullptr;
}

std::shared_ptr<BaseMaterial> MaterialManager::GetBaseMaterial(const std::string& filepath)
{
	auto baseMaterialByFilepath = m_BaseMaterialsByFilepath.find(filepath);
	if (baseMaterialByFilepath != m_BaseMaterialsByFilepath.end())
	{
		return baseMaterialByFilepath->second;
	}

	return nullptr;
}

std::shared_ptr<Material> MaterialManager::Inherit(const std::string& name, const std::string& filepath,
	std::shared_ptr<BaseMaterial> baseMaterial)
{
	std::shared_ptr<Material> inheritedMaterial = Material::Inherit(name, filepath, baseMaterial);
	m_MaterialsByFilepath[filepath] = inheritedMaterial;

	return inheritedMaterial;
}

std::shared_ptr<Material> MaterialManager::Clone(const std::string& name, const std::string& filepath, std::shared_ptr<Material> material)
{
	std::shared_ptr<Material> clonedMaterial = Material::Clone(name, filepath, material);
	m_MaterialsByFilepath[filepath] = clonedMaterial;

	return clonedMaterial;
}

void MaterialManager::ShutDown()
{
	m_MaterialsByFilepath.clear();
	m_BaseMaterialsByFilepath.clear();
}

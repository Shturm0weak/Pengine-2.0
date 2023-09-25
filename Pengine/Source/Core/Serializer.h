#pragma once

#include "Core.h"

#include "../Configs/EngineConfig.h"
#include "../Graphics/Pipeline.h"
#include "../Graphics/Material.h"
#include "../Graphics/Mesh.h"

namespace Pengine
{

	class PENGINE_API Serializer
	{
	public:
		static EngineConfig DeserializeEngineConfig(const std::string& filepath);

		static void GenerateFilesUUID(const std::filesystem::path& directory);

		static const std::string GenerateFileUUID(const std::string& filepath);

		//static void SerializeUUID();

		//static void LoadFilesUUID();

		static std::vector<Pipeline::CreateInfo> LoadBaseMaterial(const std::string& filepath);

		static Material::CreateInfo LoadMaterial(const std::string& filepath);

		static void SerializeMesh(const std::string& directory, std::shared_ptr<Mesh> mesh);

		static std::shared_ptr<Mesh> DeserializeMesh(const std::string& filepath);

		static void SerializeMeshes(const std::string& filepath, std::vector<std::shared_ptr<Mesh>> meshes);

		static std::vector<std::shared_ptr<Mesh>> DeserializeMeshes(const std::string& filepath);

		static void SerializeShaderCache(std::string filepath, const std::string& code);

		static std::string DeserializeShaderCache(std::string filepath);
	};

}
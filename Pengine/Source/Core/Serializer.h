#pragma once

#include "Core.h"
#include "Entity.h"

#include "../Configs/EngineConfig.h"
#include "../Graphics/Pipeline.h"
#include "../Graphics/Material.h"
#include "../Graphics/Mesh.h"

#include "yaml-cpp/yaml.h"

class aiMesh;
class aiNode;
class aiMaterial;

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

		static void SerializeMaterial(std::shared_ptr<Material> material);

		static void SerializeMesh(const std::string& directory, std::shared_ptr<Mesh> mesh);

		static std::shared_ptr<Mesh> DeserializeMesh(const std::string& filepath);

		static void SerializeShaderCache(std::string filepath, const std::string& code);

		static std::string DeserializeShaderCache(std::string filepath);

		static std::unordered_map<std::shared_ptr<Material>, std::vector<std::shared_ptr<Mesh>>> LoadIntermediate(const std::string& filepath);

		static std::shared_ptr<Mesh> GenerateMesh(aiMesh* aiMesh, const std::string& directory);

		static std::shared_ptr<Material> GenerateMaterial(aiMaterial* aiMaterial, const std::string& directory);

		static std::shared_ptr<Entity> GenerateEntity(aiNode* aiNode,
			const std::unordered_map<size_t, std::shared_ptr<Mesh>>& meshesByIndex,
			const std::unordered_map<std::shared_ptr<Mesh>, std::shared_ptr<Material>>& materialsByMeshes);

		static void SerializeEntity(YAML::Emitter& out, std::shared_ptr<Entity> entity, bool withChilds = true);

		static std::shared_ptr<Entity> DeserializeEntity(const YAML::Node& in, std::shared_ptr<Scene> scene,
			std::vector<std::string>& childs);

		static void SerializePrefab(const std::string& filepath, std::shared_ptr<Entity> entity);

		static void DeserializePrefab(const std::string& filepath, std::shared_ptr<Scene> scene);

		static void SerializeTransform(YAML::Emitter& out, std::shared_ptr<Entity> entity);

		static void DeserializeTransform(const YAML::Node& in, std::shared_ptr<Entity> entity);

		static void SerializeRenderer3D(YAML::Emitter& out, std::shared_ptr<Entity> entity);

		static void DeserializeRenderer3D(const YAML::Node& in, std::shared_ptr<Entity> entity);

		static void SerializePointLight(YAML::Emitter& out, std::shared_ptr<Entity> entity);

		static void DeserializePointLight(const YAML::Node& in, std::shared_ptr<Entity> entity);

		static void SerializeCamera(YAML::Emitter& out, std::shared_ptr<Entity> entity);

		static void DeserializeCamera(const YAML::Node& in, std::shared_ptr<Entity> entity);

		static void SerializeScene(const std::string& filepath, std::shared_ptr<Scene> scene);

		static std::shared_ptr<Scene> DeserializeScene(const std::string& filepath);
	};

}
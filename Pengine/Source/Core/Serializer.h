#pragma once

#include "Core.h"
#include "Entity.h"
#include "GraphicsSettings.h"

#include "../Configs/EngineConfig.h"
#include "../Graphics/Material.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/Pipeline.h"

#include "yaml-cpp/yaml.h"

class aiMesh;
class aiNode;
class aiMaterial;

namespace Pengine
{

	class PENGINE_API Serializer
	{
	public:
		static EngineConfig DeserializeEngineConfig(const std::filesystem::path& filepath);

		static void GenerateFilesUUID(const std::filesystem::path& directory);

		static std::string GenerateFileUUID(const std::filesystem::path& filepath);

		//static void SerializeUUID();

		//static void LoadFilesUUID();

		static BaseMaterial::CreateInfo LoadBaseMaterial(const std::filesystem::path& filepath);

		static Material::CreateInfo LoadMaterial(const std::filesystem::path& filepath);

		static void SerializeMaterial(const std::shared_ptr<Material>& material);

		static void SerializeMesh(const std::filesystem::path& directory, const std::shared_ptr<Mesh>& mesh);

		static std::shared_ptr<Mesh> DeserializeMesh(const std::filesystem::path& filepath);

		static void SerializeShaderCache(const std::filesystem::path& filepath, const std::string& code);

		static std::string DeserializeShaderCache(const std::filesystem::path& filepath);

		static void SerializeShaderModuleReflection(const std::filesystem::path& filepath, const ShaderReflection::ReflectShaderModule& reflectShaderModule);

		static std::optional<ShaderReflection::ReflectShaderModule> DeserializeShaderModuleReflection(const std::filesystem::path& filepath);

		static std::unordered_map<std::shared_ptr<Material>, std::vector<std::shared_ptr<Mesh>>> LoadIntermediate(const std::filesystem::path& filepath);

		static std::shared_ptr<Mesh> GenerateMesh(aiMesh* aiMesh, const std::filesystem::path& directory);

		static std::shared_ptr<Material> GenerateMaterial(const aiMaterial* aiMaterial, const std::filesystem::path& directory);

		static std::shared_ptr<Entity> GenerateEntity(const aiNode* aiNode,
			const std::unordered_map<size_t, std::shared_ptr<Mesh>>& meshesByIndex,
			const std::unordered_map<std::shared_ptr<Mesh>, std::shared_ptr<Material>>& materialsByMeshes);

		static void SerializeEntity(YAML::Emitter& out, const std::shared_ptr<Entity>& entity, bool withChilds = true);

		static std::shared_ptr<Entity> DeserializeEntity(const YAML::Node& in, const std::shared_ptr<Scene>& scene,
			std::vector<std::string>& childs);

		static void SerializePrefab(const std::filesystem::path& filepath, const std::shared_ptr<Entity>& entity);

		static void DeserializePrefab(const std::filesystem::path& filepath, const std::shared_ptr<Scene>& scene);

		static void SerializeTransform(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializeTransform(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializeRenderer3D(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializeRenderer3D(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializePointLight(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializePointLight(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializeDirectionalLight(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializeDirectionalLight(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializeCamera(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializeCamera(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializeScene(const std::filesystem::path& filepath, const std::shared_ptr<Scene>& scene);

		static std::shared_ptr<Scene> DeserializeScene(const std::filesystem::path& filepath);

		static void SerializeGraphicsSettings(const GraphicsSettings& graphicsSettings);

		static GraphicsSettings DeserializeGraphicsSettings(const std::filesystem::path& filepath);

	private:
		static void ParseUniformValues(
			const YAML::detail::iterator_value& data,
			Pipeline::UniformInfo& uniformsInfo);
	};

}
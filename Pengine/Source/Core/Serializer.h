#pragma once

#include "Core.h"
#include "Entity.h"
#include "GraphicsSettings.h"

#include "../Configs/EngineConfig.h"
#include "../Graphics/SkeletalAnimation.h"
#include "../Graphics/Material.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/Pipeline.h"
#include "../Graphics/Skeleton.h"

#include "yaml-cpp/yaml.h"

class aiMesh;
class aiNode;
class aiMaterial;
class aiAnimation;

namespace Pengine
{

	class PENGINE_API Serializer
	{
	public:
		static EngineConfig DeserializeEngineConfig(const std::filesystem::path& filepath);

		static void GenerateFilesUUID(const std::filesystem::path& directory);

		static UUID GenerateFileUUID(const std::filesystem::path& filepath);

		//static void SerializeUUID();

		//static void LoadFilesUUID();

		static void DeserializeDescriptorSets(
			const YAML::detail::iterator_value& pipelineData,
			const std::string& passName,
			std::map<Pipeline::DescriptorSetIndexType, std::map<std::string, uint32_t>>& descriptorSetIndicesByType);

		static void DeserializeShaderFilepaths(
			const YAML::detail::iterator_value& pipelineData,
			std::map<ShaderModule::Type, std::filesystem::path>& shaderFilepathsByType);

		static void SerializeTexture(const std::filesystem::path& filepath, std::shared_ptr<Texture> texture, bool* isLoaded);

		static GraphicsPipeline::CreateGraphicsInfo DeserializeGraphicsPipeline(const YAML::detail::iterator_value& pipelineData);

		static ComputePipeline::CreateComputeInfo DeserializeComputePipeline(const YAML::detail::iterator_value& pipelineData);

		static BaseMaterial::CreateInfo LoadBaseMaterial(const std::filesystem::path& filepath);

		static Material::CreateInfo LoadMaterial(const std::filesystem::path& filepath);

		static void SerializeMaterial(const std::shared_ptr<Material>& material, bool useLog = true);

		static void SerializeMesh(const std::filesystem::path& directory, const std::shared_ptr<Mesh>& mesh);

		static Mesh::CreateInfo DeserializeMesh(const std::filesystem::path& filepath);

		static void SerializeSkeleton(const std::shared_ptr<Skeleton>& skeleton);

		static std::shared_ptr<Skeleton> DeserializeSkeleton(const std::filesystem::path& filepath);

		static void SerializeSkeletalAnimation(const std::shared_ptr<SkeletalAnimation>& skeletalAnimation);

		static std::shared_ptr<SkeletalAnimation> DeserializeSkeletalAnimation(const std::filesystem::path& filepath);

		static void SerializeShaderCache(const std::filesystem::path& filepath, const std::string& code);

		static std::string DeserializeShaderCache(const std::filesystem::path& filepath);

		static void SerializeShaderModuleReflection(const std::filesystem::path& filepath, const ShaderReflection::ReflectShaderModule& reflectShaderModule);

		static std::optional<ShaderReflection::ReflectShaderModule> DeserializeShaderModuleReflection(const std::filesystem::path& filepath);

		static std::unordered_map<std::shared_ptr<Material>, std::vector<std::shared_ptr<Mesh>>> LoadIntermediate(
			const std::filesystem::path& filepath,
			const bool importMeshes,
			const bool importMaterials,
			const bool importSkeletons,
			const int flags,
			std::string& workName,
			float& workStatus);

		static std::shared_ptr<Mesh> GenerateMesh(aiMesh* aiMesh, const std::filesystem::path& directory);

		static std::shared_ptr<Mesh> GenerateMeshSkinned(const std::shared_ptr<Skeleton>& skeleton, aiMesh* aiMesh, const std::filesystem::path& directory);

		static std::shared_ptr<Skeleton> GenerateSkeleton(const aiNode* aiRootNode, aiMesh* aiMesh, const std::filesystem::path& directory);

		static std::shared_ptr<SkeletalAnimation> GenerateAnimation(aiAnimation* aiAnimation, const std::filesystem::path& directory);

		static std::shared_ptr<Material> GenerateMaterial(const aiMaterial* aiMaterial, const std::filesystem::path& directory);

		static std::shared_ptr<Entity> GenerateEntity(
			const aiNode* aiNode,
			const std::shared_ptr<Scene>& scene,
			const std::vector<std::shared_ptr<Mesh>>& meshesByIndex,
			const std::unordered_map<std::shared_ptr<Mesh>, std::shared_ptr<Material>>& materialsByMeshes);

		static void SerializeEntity(YAML::Emitter& out, const std::shared_ptr<Entity>& entity, bool rootEntity = false, bool isSerializingPrefab = false);

		static std::shared_ptr<Entity> DeserializeEntity(const YAML::Node& in, const std::shared_ptr<Scene>& scene);

		static void SerializePrefab(const std::filesystem::path& filepath, const std::shared_ptr<Entity>& entity);

		static std::shared_ptr<Entity> DeserializePrefab(const std::filesystem::path& filepath, const std::shared_ptr<Scene>& scene);

		static void UpdatePrefab(const std::filesystem::path& filepath, const std::vector<std::shared_ptr<Entity>> entities);

		static void SerializeTransform(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializeTransform(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializeRenderer3D(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializeRenderer3D(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializePointLight(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializePointLight(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializeDirectionalLight(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializeDirectionalLight(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializeSkeletalAnimator(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializeSkeletalAnimator(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializeCamera(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializeCamera(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializeCanvas(YAML::Emitter& out, const std::shared_ptr<Entity>& entity);

		static void DeserializeCanvas(const YAML::Node& in, const std::shared_ptr<Entity>& entity);

		static void SerializeScene(const std::filesystem::path& filepath, const std::shared_ptr<Scene>& scene);

		static std::shared_ptr<Scene> DeserializeScene(const std::filesystem::path& filepath);

		static void SerializeGraphicsSettings(const GraphicsSettings& graphicsSettings);

		static GraphicsSettings DeserializeGraphicsSettings(const std::filesystem::path& filepath);

		static void SerializeTextureMeta(const Texture::Meta& meta);

		static std::optional<Texture::Meta> DeserializeTextureMeta(const std::filesystem::path& filepath);

		static std::filesystem::path DeserializeFilepath(const std::string& uuidOrFilepath);

		static void SerializeThumbnailMeta(const std::filesystem::path& filepath, const size_t lastWriteTime);

		static size_t DeserializeThumbnailMeta(const std::filesystem::path& filepath);
	private:
		static void ParseUniformValues(
			const YAML::detail::iterator_value& data,
			Pipeline::UniformInfo& uniformsInfo);
	};

}

#pragma once

#include "../Core/Core.h"

#include "RenderPass.h"
#include "UniformLayout.h"
#include "ShaderReflection.h"

namespace Pengine
{

	class PENGINE_API Pipeline
	{
	public:
		enum class CullMode
		{
			NONE,
			FRONT,
			BACK,
			FRONT_AND_BACK,
		};

		enum class PolygonMode
		{
			FILL,
			LINE
		};

		enum class ShaderType
		{
			VERTEX,
			FRAGMENT
		};

		struct BlendStateAttachment
		{
			// TODO: fill this struct!!!
			bool blendEnabled = false;
		};

		enum class DescriptorSetIndexType
		{
			RENDERPASS,
			BASE_MATERIAL,
			MATERIAL
		};

		enum class InputRate
		{
			VERTEX,
			INSTANCE
		};

		struct BindingDescription
		{
			uint32_t binding;
			InputRate inputRate;
			std::vector<std::string> names;
		};

		struct UniformBufferInfo
		{
			std::unordered_map<std::string, int> intValuesByName;
			std::unordered_map<std::string, float> floatValuesByName;
			std::unordered_map<std::string, glm::vec2> vec2ValuesByName;
			std::unordered_map<std::string, glm::vec3> vec3ValuesByName;
			std::unordered_map<std::string, glm::vec4> vec4ValuesByName;
		};

		struct UniformInfo
		{
			std::unordered_map<std::string, UniformBufferInfo> uniformBuffersByName;
			std::unordered_map<std::string, std::string> texturesByName;
		};

		struct CreateInfo
		{
			std::vector<BindingDescription> bindingDescriptions;
			std::map<DescriptorSetIndexType, uint32_t> descriptorSetIndicesByType;
			std::vector<BlendStateAttachment> colorBlendStateAttachments;
			std::shared_ptr<RenderPass> renderPass;
			UniformInfo uniformInfo;
			std::string vertexFilepath;
			std::string fragmentFilepath;
			CullMode cullMode;
			PolygonMode polygonMode;
			bool depthTest;
			bool depthWrite;
		};

		static std::shared_ptr<Pipeline> Create(const CreateInfo& pipelineCreateInfo);

		explicit Pipeline(const CreateInfo& pipelineCreateInfo);
		virtual ~Pipeline() = default;
		Pipeline(const Pipeline&) = delete;
		Pipeline(Pipeline&&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;
		Pipeline& operator=(Pipeline&&) = delete;

		std::shared_ptr<UniformLayout> GetUniformLayout(const uint32_t descriptorSet) const;

		const std::map<uint32_t, std::shared_ptr<UniformLayout>>& GetUniformLayouts() const { return m_UniformLayoutsByDescriptorSet; }

		const std::optional<uint32_t> GetDescriptorSetIndexByType(const DescriptorSetIndexType type) const;

		const CreateInfo& GetCreateInfo() const { return m_CreateInfo; }

	protected:

		CreateInfo m_CreateInfo;

		ShaderReflection::ReflectShaderModule m_ReflectVertexShaderModule;
		ShaderReflection::ReflectShaderModule m_ReflectFragmentShaderModule;
		std::map<uint32_t, std::shared_ptr<UniformLayout>> m_UniformLayoutsByDescriptorSet;
	};

}
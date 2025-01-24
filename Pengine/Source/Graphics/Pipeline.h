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
		enum class TopologyMode
		{
			LINE_LIST,
			POINT_LIST,
			TRIANGLE_LIST
		};

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
			GEOMETRY,
			FRAGMENT,
			COMPUTE
		};

		struct BlendStateAttachment
		{
			enum class BlendOp
			{
				ADD = 0,
				SUBTRACT = 1,
				REVERSE_SUBTRACT = 2,
				MIN = 3,
				MAX = 4,
			};

			enum class BlendFactor
			{
				ZERO = 0,
				ONE = 1,
				SRC_COLOR = 2,
				ONE_MINUS_SRC_COLOR = 3,
				DST_COLOR = 4,
				ONE_MINUS_DST_COLOR = 5,
				SRC_ALPHA = 6,
				ONE_MINUS_SRC_ALPHA = 7,
				DST_ALPHA = 8,
				ONE_MINUS_DST_ALPHA = 9,
				CONSTANT_COLOR = 10,
				ONE_MINUS_CONSTANT_COLOR = 11,
				CONSTANT_ALPHA = 12,
				ONE_MINUS_CONSTANT_ALPHA = 13,
				SRC_ALPHA_SATURATE = 14,
			};

			bool blendEnabled = false;
			BlendFactor srcColorBlendFactor = BlendFactor::SRC_ALPHA;
			BlendFactor dstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
			BlendOp colorBlendOp = BlendOp::ADD;
			BlendFactor srcAlphaBlendFactor = BlendFactor::ONE;
			BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO;
			BlendOp alphaBlendOp = BlendOp::ADD;
		};

		enum class DescriptorSetIndexType
		{
			RENDERER,
			RENDERPASS,
			BASE_MATERIAL,
			MATERIAL,
			OBJECT,
			TOTAL
		};

		enum class InputRate
		{
			VERTEX,
			INSTANCE
		};

		enum class DepthCompare
		{
			NEVER = 0,
			LESS = 1,
			EQUAL = 2,
			LESS_OR_EQUAL = 3,
			GREATER = 4,
			NOT_EQUAL = 5,
			GREATER_OR_EQUAL = 6,
			ALWAYS = 7
		};

		struct BindingDescription
		{
			uint32_t binding;
			InputRate inputRate;
			std::vector<std::string> names;
			std::string tag;
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
			std::unordered_map<std::string, UniformLayout::RenderTargetInfo> renderTargetsByName;
		};

		struct CreateInfo
		{
			std::vector<BindingDescription> bindingDescriptions;
			std::map<DescriptorSetIndexType, std::map<std::string, uint32_t>> descriptorSetIndicesByType;
			std::vector<BlendStateAttachment> colorBlendStateAttachments;
			std::shared_ptr<RenderPass> renderPass;
			UniformInfo uniformInfo;
			std::map<Pipeline::ShaderType, std::string> shaderFilepathsByType;
			CullMode cullMode = CullMode::NONE;
			PolygonMode polygonMode = PolygonMode::FILL;
			TopologyMode topologyMode = TopologyMode::TRIANGLE_LIST;
			bool depthTest = true;
			bool depthWrite = true;
			bool depthClamp = false;
			DepthCompare depthCompare = DepthCompare::LESS_OR_EQUAL;
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

		std::map<std::string, uint32_t> GetDescriptorSetIndexByType(const DescriptorSetIndexType type) const;

		const std::optional<uint32_t> GetDescriptorSetIndexByType(const DescriptorSetIndexType type, const std::string& renderPassName) const;

		const std::map<uint32_t, std::pair<DescriptorSetIndexType, std::string>>& GetSortedDescriptorSets() const { return m_SortedDescriptorSets;  }

		const CreateInfo& GetCreateInfo() const { return m_CreateInfo; }

	protected:

		CreateInfo m_CreateInfo;

		std::map<ShaderType, ShaderReflection::ReflectShaderModule> m_ReflectShaderModulesByType;
		std::map<uint32_t, std::shared_ptr<UniformLayout>> m_UniformLayoutsByDescriptorSet;
		std::map<uint32_t, std::pair<DescriptorSetIndexType, std::string>> m_SortedDescriptorSets;
	};

}
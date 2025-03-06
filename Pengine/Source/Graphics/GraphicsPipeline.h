#pragma once

#include "../Core/Core.h"

#include "Pipeline.h"

namespace Pengine
{

	class PENGINE_API GraphicsPipeline : public Pipeline
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

		struct CreateGraphicsInfo
		{
			Type type;
			std::map<DescriptorSetIndexType, std::map<std::string, uint32_t>> descriptorSetIndicesByType;
			std::map<Pipeline::ShaderType, std::string> shaderFilepathsByType;
			UniformInfo uniformInfo;
			std::vector<BindingDescription> bindingDescriptions;
			std::vector<BlendStateAttachment> colorBlendStateAttachments;
			std::shared_ptr<RenderPass> renderPass;
			CullMode cullMode = CullMode::NONE;
			PolygonMode polygonMode = PolygonMode::FILL;
			TopologyMode topologyMode = TopologyMode::TRIANGLE_LIST;
			bool depthTest = true;
			bool depthWrite = true;
			bool depthClamp = false;
			DepthCompare depthCompare = DepthCompare::LESS_OR_EQUAL;
		};

		static std::shared_ptr<Pipeline> Create(const CreateGraphicsInfo& createGraphicsInfo);

		explicit GraphicsPipeline(const CreateGraphicsInfo& createGraphicsInfo);
		virtual ~GraphicsPipeline() override = default;
		GraphicsPipeline(const GraphicsPipeline&) = delete;
		GraphicsPipeline(GraphicsPipeline&&) = delete;
		GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
		GraphicsPipeline& operator=(GraphicsPipeline&&) = delete;

		const CreateGraphicsInfo& GetCreateInfo() const { return m_CreateInfo; }

		virtual std::map<std::string, uint32_t> GetDescriptorSetIndexByType(const DescriptorSetIndexType type) const override;

	private:
		CreateGraphicsInfo m_CreateInfo;
	};

}

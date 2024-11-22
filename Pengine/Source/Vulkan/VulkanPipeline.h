#pragma once

#include "../Core/Core.h"
#include "../Graphics/Pipeline.h"

#include <vulkan/vulkan.h>
#include <SPIRV-Reflect/spirv_reflect.h>

#include <shaderc/shaderc.hpp>

namespace Pengine::Vk
{

	struct PipelineConfigInfo
	{
		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};

	class PENGINE_API VulkanPipeline final : public Pipeline
	{
	public:
		static void DefaultPipelineConfigInfo(PipelineConfigInfo& pipelineConfigInfo);

		void Bind(VkCommandBuffer commandBuffer) const;

		explicit VulkanPipeline(const CreateInfo& pipelineCreateInfo);
		virtual ~VulkanPipeline() override;
		VulkanPipeline(const VulkanPipeline&) = delete;
		VulkanPipeline& operator=(const VulkanPipeline&) = delete;

		static VkCullModeFlagBits ConvertCullMode(CullMode cullMode);

		static CullMode ConvertCullMode(VkCullModeFlagBits cullMode);

		static VkPrimitiveTopology ConvertTopologyMode(TopologyMode topologyMode);

		static TopologyMode ConvertTopologyMode(VkPrimitiveTopology topologyMode);

		static VkPolygonMode ConvertPolygonMode(PolygonMode polygonMode);

		static PolygonMode ConvertPolygonMode(VkPolygonMode polygonMode);

		static VkShaderStageFlagBits ConvertShaderStage(ShaderType stage);

		static ShaderType ConvertShaderStage(VkShaderStageFlagBits stage);

		VkPipeline GetPipeline() const { return m_GraphicsPipeline; }

		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

	private:
		static std::vector<VkVertexInputBindingDescription> CreateBindingDescriptions(
			const ShaderReflection::ReflectShaderModule& reflectShaderModule,
			const std::vector<BindingDescription>& bindingDescriptions);

		static std::vector<VkVertexInputAttributeDescription> CreateAttributeDescriptions(
			const ShaderReflection::ReflectShaderModule& reflectShaderModule,
			const std::vector<BindingDescription>& bindingDescriptions);

		static VkVertexInputRate ConvertVertexInputRate(InputRate vertexInputRate);

		static InputRate ConvertVertexInputRate(VkVertexInputRate vertexInputRate);

		static void CreateShaderModule(const std::string& code, VkShaderModule* shaderModule);

		static std::string CompileShaderModule(
			const std::string& filepath,
			shaderc::CompileOptions options,
			ShaderType type,
			bool useCache = true,
			bool useLog = true);

		static ShaderReflection::ReflectShaderModule Reflect(const std::string& filepath, ShaderType type);

		static void ReflectDescriptorSets(
			SpvReflectShaderModule& reflectModule,
			ShaderReflection::ReflectShaderModule& reflectShaderModule);

		static void ReflectInputVariables(
			SpvReflectShaderModule& reflectModule,
			ShaderReflection::ReflectShaderModule& reflectShaderModule);

		void CreateDescriptorSetLayouts(const ShaderReflection::ReflectShaderModule& reflectShaderModule);

		VkPipeline m_GraphicsPipeline{};
		VkPipelineLayout m_PipelineLayout{};
	};

}
#include "VulkanComputePipeline.h"

#include "VulkanDevice.h"
#include "VulkanPipelineUtils.h"
#include "VulkanUniformLayout.h"
#include "ShaderIncluder.h"

#include "../Core/Logger.h"

using namespace Pengine;
using namespace Vk;

VulkanComputePipeline::VulkanComputePipeline(const CreateComputeInfo& createComputeInfo)
	: ComputePipeline(createComputeInfo)
{
	const auto filepath = createComputeInfo.shaderFilepathsByType.at(ShaderType::COMPUTE);

	if (filepath.empty())
	{
		FATAL_ERROR("Failed to create compute pipeline, compute shader filepath is empty!");
	}

	shaderc::CompileOptions options{};
	options.SetOptimizationLevel(shaderc_optimization_level_zero);
	options.SetIncluder(std::make_unique<ShaderIncluder>());

	const std::string vertexSpv = VulkanPipelineUtils::CompileShaderModule(filepath, options, ShaderType::COMPUTE);
	m_ReflectShaderModulesByType[ShaderType::COMPUTE] = VulkanPipelineUtils::Reflect(filepath, ShaderType::COMPUTE);
	VkShaderModule shaderModule{};
	VulkanPipelineUtils::CreateShaderModule(vertexSpv, &shaderModule);
	m_UniformLayoutsByDescriptorSet = VulkanPipelineUtils::CreateDescriptorSetLayouts(m_ReflectShaderModulesByType[ShaderType::COMPUTE]);

	VkPipelineShaderStageCreateInfo shaderStage{};

	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = VulkanPipelineUtils::ConvertShaderStage(ShaderType::COMPUTE);
	shaderStage.module = shaderModule;
	shaderStage.pName = "main";
	shaderStage.flags = 0;
	shaderStage.pNext = nullptr;
	shaderStage.pSpecializationInfo = nullptr;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	for (const auto& [set, uniformLayout] : m_UniformLayoutsByDescriptorSet)
	{
		descriptorSetLayouts.emplace_back(std::static_pointer_cast<VulkanUniformLayout>(uniformLayout)->GetDescriptorSetLayout());
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutCreateInfo,
		nullptr, &m_PipelineLayout) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create pipeline layout!");
	}

	VkComputePipelineCreateInfo vkComputePipelineCreateInfo{};
	vkComputePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	vkComputePipelineCreateInfo.layout = m_PipelineLayout;
	vkComputePipelineCreateInfo.stage = shaderStage;

	vkComputePipelineCreateInfo.basePipelineIndex = -1;
	vkComputePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1,
		&vkComputePipelineCreateInfo, nullptr, &m_ComputePipeline) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device->GetDevice(), shaderModule, nullptr);
}

VulkanComputePipeline::~VulkanComputePipeline()
{
	device->DeleteResource([pipelineLayout = m_PipelineLayout, computePipeline = m_ComputePipeline]()
	{
		vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);
		vkDestroyPipeline(device->GetDevice(), computePipeline, nullptr);
	});
}

void VulkanComputePipeline::Bind(VkCommandBuffer commandBuffer) const
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
}

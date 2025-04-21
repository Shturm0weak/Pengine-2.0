#include "VulkanComputePipeline.h"

#include "VulkanDevice.h"
#include "VulkanPipelineUtils.h"
#include "VulkanUniformLayout.h"
#include "VulkanShaderModule.h"

#include "../Core/Logger.h"
#include "../Graphics/ShaderModuleManager.h"

using namespace Pengine;
using namespace Vk;

VulkanComputePipeline::VulkanComputePipeline(const CreateComputeInfo& createComputeInfo)
	: ComputePipeline(createComputeInfo)
{
	const auto filepath = createComputeInfo.shaderFilepathsByType.at(ShaderModule::Type::COMPUTE);

	if (filepath.empty())
	{
		FATAL_ERROR("Failed to create compute pipeline, compute shader filepath is empty!");
	}

	const std::shared_ptr<ShaderModule> shaderModule = ShaderModuleManager::GetInstance().GetOrCreateShaderModule(filepath, ShaderModule::Type::COMPUTE);
	m_ShaderModulesByType[ShaderModule::Type::COMPUTE] = shaderModule;

	std::map<uint32_t, std::vector<ShaderReflection::ReflectDescriptorSetBinding>> bindingsByDescriptorSet;
	for (const auto& [set, bindings] : std::static_pointer_cast<VulkanShaderModule>(shaderModule)->GetReflection().setLayouts)
	{
		bindingsByDescriptorSet[set] = bindings;
	}

	m_UniformLayoutsByDescriptorSet = VulkanPipelineUtils::CreateDescriptorSetLayouts(bindingsByDescriptorSet);

	VkPipelineShaderStageCreateInfo shaderStage{};

	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = VulkanPipelineUtils::ConvertShaderStage(ShaderModule::Type::COMPUTE);
	shaderStage.module = std::static_pointer_cast<VulkanShaderModule>(shaderModule)->GetShaderModule();
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

	if (vkCreatePipelineLayout(GetVkDevice()->GetDevice(), &pipelineLayoutCreateInfo,
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

	if (vkCreateComputePipelines(GetVkDevice()->GetDevice(), VK_NULL_HANDLE, 1,
		&vkComputePipelineCreateInfo, nullptr, &m_ComputePipeline) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create graphics pipeline!");
	}
}

VulkanComputePipeline::~VulkanComputePipeline()
{
	for (auto& [type, shaderModule] : m_ShaderModulesByType)
	{
		ShaderModuleManager::GetInstance().DeleteShaderModule(shaderModule);
	}
	m_ShaderModulesByType.clear();

	GetVkDevice()->DeleteResource([pipelineLayout = m_PipelineLayout, computePipeline = m_ComputePipeline]()
	{
		vkDestroyPipelineLayout(GetVkDevice()->GetDevice(), pipelineLayout, nullptr);
		vkDestroyPipeline(GetVkDevice()->GetDevice(), computePipeline, nullptr);
	});
}

void VulkanComputePipeline::Bind(VkCommandBuffer commandBuffer) const
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
}

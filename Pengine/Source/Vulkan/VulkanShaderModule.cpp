#include "VulkanShaderModule.h"

#include "VulkanDevice.h"
#include "VulkanPipelineUtils.h"
#include "ShaderIncluder.h"

using namespace Pengine;
using namespace Vk;

VulkanShaderModule::VulkanShaderModule(
	const std::filesystem::path& filepath,
	const Type type)
	: ShaderModule(filepath, type)
{
	shaderc::CompileOptions options{};
	options.SetOptimizationLevel(shaderc_optimization_level_zero);
	options.SetIncluder(std::make_unique<ShaderIncluder>());

	const std::string spv = VulkanPipelineUtils::CompileShaderModule(filepath, options, type);
	m_Reflection = VulkanPipelineUtils::Reflect(filepath, type);
	VulkanPipelineUtils::CreateShaderModule(spv, &m_ShaderModule);
}

VulkanShaderModule::~VulkanShaderModule()
{
	GetVkDevice()->DeleteResource([shaderModule = m_ShaderModule]()
	{
		vkDestroyShaderModule(GetVkDevice()->GetDevice(), shaderModule, nullptr);
	});
}

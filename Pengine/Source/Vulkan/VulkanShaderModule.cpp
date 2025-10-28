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
	Reload();
}

VulkanShaderModule::~VulkanShaderModule()
{
	GetVkDevice()->DeleteResource([shaderModule = m_ShaderModule]()
	{
		vkDestroyShaderModule(GetVkDevice()->GetDevice(), shaderModule, nullptr);
	});
}

void VulkanShaderModule::Reload(bool useCache)
{
	shaderc::CompileOptions options{};
	options.SetOptimizationLevel(shaderc_optimization_level_zero);
	options.SetIncluder(std::make_unique<ShaderIncluder>());

	const std::string spv = VulkanPipelineUtils::CompileShaderModule(GetFilepath(), options, GetType(), useCache);
	if (spv.empty())
	{
		return;
	}

	if (auto reflection = VulkanPipelineUtils::Reflect(GetFilepath(), GetType(), useCache))
	{
		m_Reflection = *reflection;
	}
	else
	{
		return;
	}

	VulkanPipelineUtils::CreateShaderModule(spv, &m_ShaderModule);
}

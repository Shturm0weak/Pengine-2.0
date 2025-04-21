#include "ShaderModule.h"

#include "../Core/Logger.h"

#include "../Vulkan/VulkanShaderModule.h"

#include "../Utils/Utils.h"

using namespace Pengine;

std::shared_ptr<ShaderModule> ShaderModule::Create(
	const std::filesystem::path& filepath,
	const Type type)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanShaderModule>(filepath, type);
	}

	FATAL_ERROR("Failed to create the shader module, no graphics API implementation");
	return nullptr;
}

ShaderModule::ShaderModule(
	const std::filesystem::path& filepath,
	const Type type)
	: Asset(Utils::GetFilename(filepath), filepath)
	, m_Type(type)
{
}

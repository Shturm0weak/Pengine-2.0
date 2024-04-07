#include "UniformWriter.h"

#include "../Core/Logger.h"
#include "../Utils/Utils.h"
#include "../Vulkan/VulkanUniformWriter.h"

using namespace Pengine;

std::shared_ptr<UniformWriter> UniformWriter::Create(std::shared_ptr<UniformLayout> uniformLayout)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanUniformWriter>(uniformLayout);
	}

	FATAL_ERROR("Failed to create the uniform writer, no graphics API implementation");
	return nullptr;
}

UniformWriter::UniformWriter(std::shared_ptr<UniformLayout> uniformLayout)
	: m_UniformLayout(uniformLayout)
{
}

std::shared_ptr<Texture> UniformWriter::GetTexture(const std::string& name)
{
	return Utils::Find(name, m_TexturesByName);
}

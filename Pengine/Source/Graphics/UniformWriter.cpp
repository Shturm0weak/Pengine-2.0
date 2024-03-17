#include "UniformWriter.h"

#include "../Core/Logger.h"
#include "../Utils/Utils.h"
#include "../Vulkan/VulkanUniformWriter.h"

using namespace Pengine;

std::shared_ptr<UniformWriter> UniformWriter::Create(const std::shared_ptr<UniformLayout>& layout)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanUniformWriter>(layout);
	}

	FATAL_ERROR("Failed to create the uniform writer, no graphics API implementation");
	return nullptr;
}

UniformWriter::UniformWriter(const std::shared_ptr<UniformLayout>& layout)
	: m_Layout(layout)
{

}
#include "UniformWriter.h"

#include "../Utils/Utils.h"
#include "../Vulkan/VulkanUniformWriter.h"

using namespace Pengine;

std::shared_ptr<UniformWriter> UniformWriter::Create(std::shared_ptr<UniformLayout> layout)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanUniformWriter>(layout);
	}
}

UniformWriter::UniformWriter(std::shared_ptr<UniformLayout> layout)
	: m_Layout(layout)
{

}
#include "VulkanUniformWriter.h"

#include "VulkanBuffer.h"
#include "VulkanDescriptors.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "VulkanUniformLayout.h"

#include "../Core/Logger.h"

using namespace Pengine;
using namespace Vk;

VulkanUniformWriter::VulkanUniformWriter(
	std::shared_ptr<UniformLayout> uniformLayout,
	bool isMultiBuffered)
	: UniformWriter(uniformLayout, isMultiBuffered)
{
	const size_t count = IsMultiBuffered() ? swapChainImageCount : 1;

	m_DescriptorSets.resize(count);

	if (!GetVkDevice()->GetDescriptorPool()->AllocateDescriptorSets(
		std::static_pointer_cast<VulkanUniformLayout>(m_UniformLayout)->GetDescriptorSetLayout(), m_DescriptorSets))
	{
		FATAL_ERROR("Failed to allocate descriptor set!");
	}
}

VulkanUniformWriter::~VulkanUniformWriter()
{
	GetVkDevice()->DeleteResource([descriptorSets = m_DescriptorSets]()
	{
		GetVkDevice()->GetDescriptorPool()->FreeDescriptors(descriptorSets);
	});
}

void VulkanUniformWriter::Flush()
{
	const uint32_t index = IsMultiBuffered() ? swapChainImageIndex : 0;
	const VkDescriptorSet set = m_DescriptorSets[index];

	std::vector<VkWriteDescriptorSet> writes;

	size_t bufferInfoCount = 0;
	for (const auto& [location, bufferWrite] : m_Writes[index].m_BufferWritesByLocation)
	{
		bufferInfoCount += bufferWrite.buffers.size();
	}

	std::vector<VkDescriptorBufferInfo> bufferInfos;
	bufferInfos.reserve(bufferInfoCount);
	for (const auto& [location, bufferWrite] : m_Writes[index].m_BufferWritesByLocation)
	{
		for (const auto& buffer : bufferWrite.buffers)
		{
			assert(bufferInfoCount > 0);
			bufferInfos.emplace_back(std::static_pointer_cast<VulkanBuffer>(buffer)->GetDescriptorInfo(index, bufferWrite.size, bufferWrite.offset));
		}

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = VulkanUniformLayout::ConvertDescriptorType(bufferWrite.binding.type);
		write.dstBinding = location;
		write.pBufferInfo = &bufferInfos.back();
		write.descriptorCount = bufferWrite.binding.count;
		write.dstSet = set;

		writes.emplace_back(write);
	}
	m_Writes[index].m_BufferWritesByLocation.clear();

	size_t imageInfoCount = 0;
	for (const auto& [location, textureWrite] : m_Writes[index].m_TextureWritesByLocation)
	{
		imageInfoCount += textureWrite.textures.size();
	}

	std::vector<VkDescriptorImageInfo> imageInfos;
	imageInfos.reserve(imageInfoCount);
	for (const auto& [location, textureWrite] : m_Writes[index].m_TextureWritesByLocation)
	{
		for (const auto& texture : textureWrite.textures)
		{
			assert(imageInfoCount > 0);
			imageInfos.emplace_back(std::static_pointer_cast<VulkanTexture>(texture)->GetDescriptorInfo(index));
		}

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = VulkanUniformLayout::ConvertDescriptorType(textureWrite.binding.type);
		write.dstBinding = location;
		write.pImageInfo = &imageInfos.back();
		write.descriptorCount = textureWrite.binding.count;
		write.dstSet = set;

		writes.emplace_back(write);
	}
	m_Writes[index].m_TextureWritesByLocation.clear();

	if (writes.empty())
	{
		return;
	}

	vkUpdateDescriptorSets(GetVkDevice()->GetDevice(), writes.size(), writes.data(), 0, nullptr);
}

void VulkanUniformWriter::WriteTexture(uint32_t location, const std::vector<VkDescriptorImageInfo>& vkDescriptorImageInfos)
{
	const size_t count = IsMultiBuffered() ? swapChainImageCount : 1;
	assert(count == vkDescriptorImageInfos.size());

	const auto binding = m_UniformLayout->GetBindingByLocation(location);

	if (!binding)
	{
		FATAL_ERROR("Layout does not contain specified binding!");
	}

	std::vector<VkWriteDescriptorSet> writes;
	writes.resize(count);
	for (size_t i = 0; i < count; i++)
	{
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = VulkanUniformLayout::ConvertDescriptorType(binding->type);
		write.dstBinding = location;
		write.pImageInfo = &vkDescriptorImageInfos[i];
		write.descriptorCount = binding->count;
		write.dstSet = m_DescriptorSets[i];

		writes[i] = write;
	}

	vkUpdateDescriptorSets(GetVkDevice()->GetDevice(), writes.size(), writes.data(), 0, nullptr);
}

VkDescriptorSet VulkanUniformWriter::GetDescriptorSet() const
{
	return m_DescriptorSets[IsMultiBuffered() ? swapChainImageIndex : 0];
}

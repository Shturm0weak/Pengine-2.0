#include "VulkanUniformWriter.h"

#include "VulkanBuffer.h"
#include "VulkanDescriptors.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "VulkanUniformLayout.h"

#include "../Core/Logger.h"

using namespace Pengine;
using namespace Vk;

VulkanUniformWriter::VulkanUniformWriter(std::shared_ptr<UniformLayout> uniformLayout)
	: UniformWriter(uniformLayout)
{
	m_DescriptorSetWrites.resize(swapChainImageCount);
	for (auto& descriptorSetWrite : m_DescriptorSetWrites)
	{
		descriptorSetWrite.m_BufferInfos.reserve(32);
		descriptorSetWrite.m_ImageInfos.reserve(32);
	}

	m_DescriptorSets.resize(swapChainImageCount);

	if (!descriptorPool->AllocateDescriptorSets(
		std::static_pointer_cast<VulkanUniformLayout>(m_UniformLayout)->GetDescriptorSetLayout(), m_DescriptorSets))
	{
		FATAL_ERROR("Failed to allocate descriptor set!");
	}
}

VulkanUniformWriter::~VulkanUniformWriter()
{
	vkDeviceWaitIdle(device->GetDevice());
	descriptorPool->FreeDescriptors(m_DescriptorSets);
}

void VulkanUniformWriter::WriteBuffer(
	const uint32_t location,
	const std::shared_ptr<Buffer>& buffer,
	const size_t size,
	const size_t offset)
{
	m_BuffersByLocation[location] = buffer;

	const auto binding = m_UniformLayout->GetBindingByLocation(location);

	if (!binding)
	{
		FATAL_ERROR("Layout does not contain specified binding!");
	}

	for (size_t i = 0; i < swapChainImageCount; i++)
	{
		m_DescriptorSetWrites[i].m_BufferInfos.emplace_back(std::static_pointer_cast<VulkanBuffer>(buffer)->DescriptorInfo(i, size, offset));

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = VulkanUniformLayout::ConvertDescriptorType(binding->type);
		write.dstBinding = location;
		write.pBufferInfo = &m_DescriptorSetWrites[i].m_BufferInfos.back();
		write.descriptorCount = binding->count;

		m_DescriptorSetWrites[i].m_WritesByLocation[location] = write;
	}
}

void VulkanUniformWriter::WriteTexture(
	const uint32_t location,
	const std::shared_ptr<Texture>& texture)
{
	const auto binding = m_UniformLayout->GetBindingByLocation(location);

	if (!binding)
	{
		FATAL_ERROR("Layout does not contain specified binding!");
	}

	for (size_t i = 0; i < swapChainImageCount; i++)
	{
		m_DescriptorSetWrites[i].m_ImageInfos.emplace_back(std::static_pointer_cast<VulkanTexture>(texture)->GetDescriptorInfo());

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = VulkanUniformLayout::ConvertDescriptorType(binding->type);
		write.dstBinding = location;
		write.pImageInfo = &m_DescriptorSetWrites[i].m_ImageInfos.back();
		write.descriptorCount = binding->count;

		m_DescriptorSetWrites[i].m_WritesByLocation[location] = write;
	}
}

void VulkanUniformWriter::WriteTextures(
	const uint32_t location,
	const std::vector<std::shared_ptr<Texture>>& textures)
{
	const auto binding = m_UniformLayout->GetBindingByLocation(location);

	if (!binding)
	{
		FATAL_ERROR("Layout does not contain specified binding!");
	}

	for (size_t i = 0; i < swapChainImageCount; i++)
	{
		const size_t index = m_DescriptorSetWrites[i].m_ImageInfos.size();
		for (const auto& texture : textures)
		{
			m_DescriptorSetWrites[i].m_ImageInfos.emplace_back(std::static_pointer_cast<VulkanTexture>(texture)->GetDescriptorInfo());
		}

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = VulkanUniformLayout::ConvertDescriptorType(binding->type);
		write.dstBinding = location;
		write.pImageInfo = &m_DescriptorSetWrites[i].m_ImageInfos[index];
		write.descriptorCount = m_DescriptorSetWrites[i].m_ImageInfos.size();

		m_DescriptorSetWrites[i].m_WritesByLocation[location] = write;
	}
}

void VulkanUniformWriter::WriteBuffer(
	const std::string& name,
	const std::shared_ptr<Buffer>& buffer,
	const size_t size,
	const size_t offset)
{
	WriteBuffer(m_UniformLayout->GetBindingLocationByName(name), buffer, size, offset);
}

void VulkanUniformWriter::WriteTexture(
	const std::string& name,
	const std::shared_ptr<Texture>& texture)
{
	m_TexturesByName[name] = texture;
	WriteTexture(m_UniformLayout->GetBindingLocationByName(name), texture);
}

void VulkanUniformWriter::WriteTextures(
	const std::string& name,
	const std::vector<std::shared_ptr<Texture>>& textures)
{
	// TODO: Add the ability to contain multiple textures.
	// m_TexturesByName[name] = textures;
	WriteTextures(m_UniformLayout->GetBindingLocationByName(name), textures);
}

void VulkanUniformWriter::Flush()
{
	auto& descriptorSetWrites = m_DescriptorSetWrites[swapChainImageIndex];
	if (descriptorSetWrites.m_WritesByLocation.empty())
	{
		return;
	}

	std::vector<VkWriteDescriptorSet> writes;
	writes.reserve(descriptorSetWrites.m_WritesByLocation.size());
	for (auto& [location, write] : descriptorSetWrites.m_WritesByLocation)
	{
		write.dstSet = m_DescriptorSets[swapChainImageIndex];
		writes.emplace_back(write);
	}

	vkUpdateDescriptorSets(device->GetDevice(), writes.size(), writes.data(), 0, nullptr);

	descriptorSetWrites.m_ImageInfos.clear();
	descriptorSetWrites.m_BufferInfos.clear();
	descriptorSetWrites.m_WritesByLocation.clear();
}

VkDescriptorSet VulkanUniformWriter::GetDescriptorSet() const
{
	return m_DescriptorSets[swapChainImageIndex];
}

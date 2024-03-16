#include "VulkanUniformWriter.h"

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanDescriptors.h"
#include "VulkanUniformLayout.h"
#include "VulkanTexture.h"

#include "../Core/Logger.h"

using namespace Pengine;
using namespace Vk;

VulkanUniformWriter::VulkanUniformWriter(std::shared_ptr<UniformLayout> layout)
    : UniformWriter(layout)
{
    m_DescriptorSet.resize(Vk::swapChainImageCount);

    m_ImageInfos.reserve(MAX_TEXTURES);
    m_BufferInfos.reserve(512);
}

void VulkanUniformWriter::WriteBuffer(uint32_t location,
	std::shared_ptr<Buffer> buffer, size_t size, size_t offset)
{
    if (m_Layout->GetBindingsByLocation().count(location) == 0)
    {
        FATAL_ERROR("Layout does not contain specified binding!")
    }

    const auto& bindingDescription = m_Layout->GetBindingByLocation(location);

    m_BufferInfos.emplace_back(std::static_pointer_cast<VulkanBuffer>(buffer)->DescriptorInfo(size, offset));

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VulkanUniformLayout::ConvertDescriptorType(bindingDescription.type);
    write.dstBinding = location;
    write.pBufferInfo = &m_BufferInfos.back();
    write.descriptorCount = 1;

    m_WritesByLocation[location] = write;
}

void VulkanUniformWriter::WriteTexture(uint32_t location,
	std::shared_ptr<Texture> texture)
{
    if (m_Layout->GetBindingsByLocation().count(location) == 0)
    {
        FATAL_ERROR("Layout does not contain specified binding!")
    }

    const auto& bindingDescription = m_Layout->GetBindingByLocation(location);

    m_ImageInfos.emplace_back(std::static_pointer_cast<VulkanTexture>(texture)->GetDescriptorInfo());

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VulkanUniformLayout::ConvertDescriptorType(bindingDescription.type);
    write.dstBinding = location;
    write.pImageInfo = &m_ImageInfos.back();
    write.descriptorCount = 1;

    m_WritesByLocation[location] = write;
}

void VulkanUniformWriter::WriteTextures(uint32_t location, std::vector<std::shared_ptr<Texture>> textures)
{
    if (textures.size() > MAX_TEXTURES || m_ImageInfos.size() > MAX_TEXTURES)
    {
        FATAL_ERROR("Textures size exceeds the maximum texture slots size!")
    }

    if (m_Layout->GetBindingsByLocation().count(location) == 0)
    {
        FATAL_ERROR("Layout does not contain specified binding!")
    }

    const auto& bindingDescription = m_Layout->GetBindingByLocation(location);

    size_t const index = m_ImageInfos.size();
    for (const auto& texture : textures)
    {
        m_ImageInfos.emplace_back(std::static_pointer_cast<VulkanTexture>(texture)->GetDescriptorInfo());
    }

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VulkanUniformLayout::ConvertDescriptorType(bindingDescription.type);
    write.dstBinding = location;
    write.pImageInfo = &m_ImageInfos[index];
    write.descriptorCount = m_ImageInfos.size();

    m_WritesByLocation[location] = write;
}

void VulkanUniformWriter::WriteBuffer(const std::string& name, std::shared_ptr<Buffer> buffer, size_t size, size_t offset)
{
    WriteBuffer(m_Layout->GetBindingLocationByName(name), buffer, size, offset);
}

void VulkanUniformWriter::WriteTexture(const std::string& name, std::shared_ptr<Texture> texture)
{
    WriteTexture(m_Layout->GetBindingLocationByName(name), texture);
}

void VulkanUniformWriter::WriteTextures(const std::string& name, std::vector<std::shared_ptr<Texture>> textures)
{
    WriteTextures(m_Layout->GetBindingLocationByName(name), textures);
}

void VulkanUniformWriter::Flush()
{
    if (Vk::swapChainImageCount > m_DescriptorSet.size())
    {
        for (size_t i = 0; i < Vk::swapChainImageCount; i++)
        {
            VkDescriptorSet descriptorSet;
            if (!descriptorPool->AllocateDescriptorSet(
                std::static_pointer_cast<VulkanUniformLayout>(m_Layout)->GetDescriptorSetLayout(), descriptorSet))
            {
                FATAL_ERROR("Failed to allocate descriptor set!")
            }

            m_DescriptorSet.emplace_back(descriptorSet);
        }
    }

    if (!m_Initialized)
    {
        for (auto& descriptorSet : m_DescriptorSet)
        {
            if (!descriptorPool->AllocateDescriptorSet(
                std::static_pointer_cast<VulkanUniformLayout>(m_Layout)->GetDescriptorSetLayout(), descriptorSet))
            {
                FATAL_ERROR("Failed to allocate descriptor set!")
            }

            std::vector<VkWriteDescriptorSet> writes;
            for (auto& [location, write] : m_WritesByLocation)
            {
                write.dstSet = descriptorSet;
                writes.emplace_back(write);
            }

            vkUpdateDescriptorSets(device->GetDevice(), writes.size(), writes.data(), 0, nullptr);
        }

        m_Initialized = true;
    }
    else
    {
        std::vector<VkWriteDescriptorSet> writes;
        for (auto& [location, write] : m_WritesByLocation)
        {
            write.dstSet = m_DescriptorSet[Vk::swapChainImageIndex];
            writes.emplace_back(write);
        }

        vkUpdateDescriptorSets(device->GetDevice(), writes.size(), writes.data(), 0, nullptr);
    }

    m_ImageInfos.clear();
    m_BufferInfos.clear();
    m_WritesByLocation.clear();
}

VkDescriptorSet VulkanUniformWriter::GetDescriptorSet() const
{
    return m_DescriptorSet[Vk::swapChainImageIndex];
}

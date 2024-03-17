#pragma once

#include "../Core/Core.h"
#include "../Graphics/UniformWriter.h"

#include <vulkan/vulkan.h>

#include <set>

namespace Pengine::Vk
{

	class PENGINE_API VulkanUniformWriter final : public UniformWriter
	{
	public:
		explicit VulkanUniformWriter(const std::shared_ptr<UniformLayout>& layout);
		virtual ~VulkanUniformWriter() override = default;
		VulkanUniformWriter(const VulkanUniformWriter&) = delete;
		VulkanUniformWriter& operator=(const VulkanUniformWriter&) = delete;

		virtual void WriteBuffer(uint32_t location, const std::shared_ptr<Buffer>& buffer, size_t size = -1, size_t offset = 0) override;
		virtual void WriteTexture(uint32_t location, const std::shared_ptr<Texture>& texture) override;
		virtual void WriteTextures(uint32_t location, const std::vector<std::shared_ptr<Texture>>& textures) override;
		virtual void WriteBuffer(const std::string& name, const std::shared_ptr<Buffer>& buffer, size_t size = -1, size_t offset = 0) override;
		virtual void WriteTexture(const std::string& name, const std::shared_ptr<Texture>& texture) override;
		virtual void WriteTextures(const std::string& name, const std::vector<std::shared_ptr<Texture>>& textures) override;
		virtual void Flush() override;

		[[nodiscard]] VkDescriptorSet GetDescriptorSet() const;

	private:
		std::unordered_map<uint32_t, VkWriteDescriptorSet> m_WritesByLocation;
		std::vector<VkDescriptorImageInfo> m_ImageInfos;
		std::vector<VkDescriptorBufferInfo> m_BufferInfos;
		std::vector<VkDescriptorSet> m_DescriptorSet;
		bool m_Initialized = false;
	};

}
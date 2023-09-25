#pragma once

#include "../Core/Core.h"
#include "../Graphics/UniformWriter.h"

#include <set>

namespace Pengine
{

	namespace Vk
	{

		class PENGINE_API VulkanUniformWriter : public UniformWriter
		{
		public:
			VulkanUniformWriter(std::shared_ptr<UniformLayout> layout);
			~VulkanUniformWriter() = default;
			VulkanUniformWriter(const VulkanUniformWriter&) = delete;
			VulkanUniformWriter& operator=(const VulkanUniformWriter&) = delete;

			virtual void WriteBuffer(uint32_t location, std::shared_ptr<Buffer> buffer, size_t size = -1, size_t offset = 0) override;
			virtual void WriteTexture(uint32_t location, std::shared_ptr<Texture> texture) override;
			virtual void WriteTextures(uint32_t location, std::vector<std::shared_ptr<Texture>> textures) override;
			virtual void WriteBuffer(const std::string& name, std::shared_ptr<Buffer> buffer, size_t size = -1, size_t offset = 0) override;
			virtual void WriteTexture(const std::string& name, std::shared_ptr<Texture> texture) override;
			virtual void WriteTextures(const std::string& name, std::vector<std::shared_ptr<Texture>> textures) override;
			virtual void Flush() override;

			VkDescriptorSet GetDescriptorSet() const;

		private:
			std::unordered_map<int, VkWriteDescriptorSet> m_WritesByLocation;
			std::vector<VkDescriptorImageInfo> m_ImageInfos;
			std::vector<VkDescriptorBufferInfo> m_BufferInfos;
			std::vector<VkDescriptorSet> m_DescriptorSet;
			bool m_Initialized = false;
		};

	}

}
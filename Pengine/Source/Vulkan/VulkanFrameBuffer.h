#pragma once

#include "../Core/Core.h"
#include "../Graphics/FrameBuffer.h"
#include "../Vulkan/VulkanFrameBuffer.h"

#include <vulkan/vulkan.h>

namespace Pengine
{

	namespace Vk
	{

		class PENGINE_API VulkanFrameBuffer : public FrameBuffer
		{
		public:
			VulkanFrameBuffer(std::vector<Texture::CreateInfo> const& attachments,
				std::shared_ptr<RenderPass> renderPass);
			virtual ~VulkanFrameBuffer() override;
			VulkanFrameBuffer(const VulkanFrameBuffer&) = delete;
			VulkanFrameBuffer& operator=(const VulkanFrameBuffer&) = delete;

			VkFramebuffer GetFrameBuffer() const { return m_FrameBuffers[swapChainImageIndex]; }

			virtual std::shared_ptr<Texture> GetAttachment(size_t index) const override 
				{ return m_Attachments[swapChainImageIndex][index]; }

			virtual void Resize(const glm::ivec2& size) override;

			virtual void Clear() override;

			void TransitionToRead();

			void TransitionToColorAttachment();

		private:
			std::vector<VkFramebuffer> m_FrameBuffers;
			std::vector<std::vector<std::shared_ptr<Texture>>> m_Attachments;
		};

	}

}
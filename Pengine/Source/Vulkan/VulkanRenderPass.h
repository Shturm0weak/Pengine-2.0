#pragma once

#include "../Core/Core.h"
#include "../Graphics/RenderPass.h"

namespace Pengine
{

	namespace Vk
	{

		class PENGINE_API VulkanRenderPass : public RenderPass
		{
		public:
			VulkanRenderPass(const CreateInfo& createInfo);
			~VulkanRenderPass();
			VulkanRenderPass(const RenderPass&) = delete;
			VulkanRenderPass& operator=(const RenderPass&) = delete;

			VkRenderPass GetRenderPass() const { return m_RenderPass; }

		private:
			VkRenderPass m_RenderPass;
		};
	
	}

}
#pragma once

#include "../Core/Core.h"
#include "../Graphics/RenderPass.h"

#include <vulkan/vulkan.h>

namespace Pengine::Vk
{

	class PENGINE_API VulkanRenderPass final : public RenderPass
	{
	public:
		explicit VulkanRenderPass(const CreateInfo& createInfo);
		virtual ~VulkanRenderPass() override;
		VulkanRenderPass(const RenderPass&) = delete;
		VulkanRenderPass(RenderPass&&) = delete;
		VulkanRenderPass& operator=(const RenderPass&) = delete;
		VulkanRenderPass& operator=(RenderPass&&) = delete;

		VkRenderPass GetRenderPass() const { return m_RenderPass; }

	private:
		VkRenderPass m_RenderPass{};
	};

}
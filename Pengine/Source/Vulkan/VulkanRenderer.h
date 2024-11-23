#pragma once

#include "../Core/Core.h"
#include "../Graphics/Buffer.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Mesh.h"

#include <imgui/backends/imgui_impl_vulkan.h>

namespace Pengine::Vk
{

	class PENGINE_API VulkanRenderer final : public Renderer
	{
	public:
		VulkanRenderer();
		virtual ~VulkanRenderer() override = default;
		VulkanRenderer(const VulkanRenderer&) = delete;
		VulkanRenderer& operator=(const VulkanRenderer&) = delete;

		virtual void BeginRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo) override;

		virtual void EndRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo) override;

		virtual void Render(
			const std::shared_ptr<Buffer>& vertices,
			const std::shared_ptr<Buffer>& indices,
			const int indexCount,
			const std::shared_ptr<Pipeline>& pipeline,
			const std::shared_ptr<Buffer>& instanceBuffer,
			size_t instanceBufferOffset, size_t count,
			const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
			void* frame) override;

	private:
		static void BindBuffers(
			VkCommandBuffer commandBuffer,
			const std::shared_ptr<Buffer>& vertexBuffer,
			const std::shared_ptr<Buffer>& instanceBuffer,
			const std::shared_ptr<Buffer>& indexBuffer,
			size_t instanceBufferOffset);

		static void DrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount);
	};

}
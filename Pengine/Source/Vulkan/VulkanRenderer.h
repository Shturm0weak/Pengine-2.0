#pragma once

#include "../Core/Core.h"
#include "../Graphics/Buffer.h"
#include "../Graphics/Renderer.h"

#include <imgui/backends/imgui_impl_vulkan.h>

namespace Pengine
{

	namespace Vk
	{

		class PENGINE_API VulkanRenderer : public Renderer
		{
		public:
			VulkanRenderer(const glm::ivec2& size);
			~VulkanRenderer() = default;
			VulkanRenderer(const VulkanRenderer&) = delete;
			VulkanRenderer& operator=(const VulkanRenderer&) = delete;

		protected:
			virtual void BeginRenderPass(RenderPass::SubmitInfo renderPassSubmitInfo) override;

			virtual void EndRenderPass(RenderPass::SubmitInfo renderPassSubmitInfo) override;

			virtual void Render(std::shared_ptr<Mesh> mesh,
				std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> instanceBuffer,
				size_t instanceBufferOffset, size_t count,
				const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
				RenderPass::SubmitInfo renderPassSubmitInfo) override;

		private:
			void BindBuffers(VkCommandBuffer commandBuffer,
				std::shared_ptr<Buffer> vertexBuffer,
				std::shared_ptr<Buffer> instanceBuffer,
				std::shared_ptr<Buffer> indexBuffer,
				size_t instanceBufferOffset);

			void DrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount);
		};

	}

}
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

		virtual void BeginRenderPass(
			const RenderPass::SubmitInfo& renderPassSubmitInfo,
			const std::string& debugName = {},
			const glm::vec3& debugColor = topLevelRenderPassDebugColor) override;

		virtual void EndRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo) override;

		virtual void SetScissors(const RenderPass::Scissors& scissors, void* frame) override;

		virtual void SetViewport(const RenderPass::Viewport& viewport, void* frame) override;

		virtual void Render(
			std::vector<NativeHandle>& vertexBuffers,
			std::vector<size_t>& vertexBufferOffsets,
			const NativeHandle indexBuffer,
			const size_t indexBufferOffset,
			const int indexCount,
			const std::shared_ptr<Pipeline>& pipeline,
			const NativeHandle instanceBuffer,
			const size_t instanceBufferOffset,
			const size_t count,
			const std::vector<NativeHandle>& uniformWriters,
			void* frame) override;

		virtual void Dispatch(
			const std::shared_ptr<Pipeline>& pipeline,
			const glm::uvec3& groupCount,
			const std::vector<NativeHandle>& uniformWriters,
			void* frame) override;

		virtual void MemoryBarrierFragmentReadWrite(void* frame) override;

		virtual void BeginCommandLabel(
			const std::string& name,
			const glm::vec3& color,
			void* frame) override;

		virtual void EndCommandLabel(void* frame) override;

		virtual void ClearDepthStencilImage(
			std::shared_ptr<Texture> texture,
			const RenderPass::ClearDepth& clearDepth,
			void* frame) override;

	private:
		void BindBuffers(
			VkCommandBuffer commandBuffer,
			std::vector<NativeHandle>& vertexBuffers,
			std::vector<size_t>& vertexBufferOffsets,
			const NativeHandle instanceBuffer,
			const size_t instanceBufferOffset,
			const NativeHandle indexBuffer,
			const size_t indexBufferOffset);

		static void DrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount);

		std::shared_ptr<class Pipeline> m_Pipeline;
	};

}

#include "VulkanRenderer.h"

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanFrameBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanRenderPass.h"
#include "VulkanUniformWriter.h"
#include "VulkanWindow.h"

using namespace Pengine;
using namespace Vk;

VulkanRenderer::VulkanRenderer(const glm::ivec2& size)
	: Renderer(size)
{

}

void VulkanRenderer::Render(
	const std::shared_ptr<Mesh>& mesh,
	const std::shared_ptr<Pipeline>& pipeline,
	const std::shared_ptr<Buffer>& instanceBuffer,
	const size_t instanceBufferOffset,
	const size_t count,
	const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
	const RenderPass::SubmitInfo& renderPassSubmitInfo)
{
	const ImGui_ImplVulkanH_Frame* frame = static_cast<ImGui_ImplVulkanH_Frame*>(renderPassSubmitInfo.frame);

	std::shared_ptr<VulkanPipeline> vkPipeline;
	if (pipeline)
	{
		vkPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
	}
	else
	{
		return;
	}
	
	vkPipeline->Bind(frame->CommandBuffer);

	std::vector<VkDescriptorSet> descriptorSets;
	for (const auto& uniformWriter : uniformWriters)
	{
		descriptorSets.emplace_back(std::static_pointer_cast<VulkanUniformWriter>(
			uniformWriter)->GetDescriptorSet());
	}

	if (!descriptorSets.empty())
	{
		vkCmdBindDescriptorSets(
			frame->CommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vkPipeline->GetPipelineLayout(),
			0,
			descriptorSets.size(),
			&descriptorSets[0],
			0,
			nullptr);
	}

	vertexCount += (mesh->GetIndexCount() / 3) * count;
	BindBuffers(frame->CommandBuffer, mesh->GetVertices(), instanceBuffer, mesh->GetIndices(), instanceBufferOffset);
	DrawIndexed(frame->CommandBuffer, mesh->GetIndexCount(), count);
}

void VulkanRenderer::BeginRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo)
{
	const ImGui_ImplVulkanH_Frame* frame = static_cast<ImGui_ImplVulkanH_Frame*>(renderPassSubmitInfo.frame);
 
	Vk::device->CommandBeginLabel(renderPassSubmitInfo.renderPass->GetType(), frame->CommandBuffer, { 0.5f, 1.0f, 0.5f, 1.0f });

	std::vector<VkClearValue> vkClearValues;
	for (const glm::vec4& clearColor : renderPassSubmitInfo.renderPass->GetClearColors())
	{
		VkClearValue clearValue{};

		clearValue.color.float32[0] = clearColor[0];
		clearValue.color.float32[1] = clearColor[1];
		clearValue.color.float32[2] = clearColor[2];
		clearValue.color.float32[3] = clearColor[3];
		vkClearValues.emplace_back(clearValue);
	}

	for (const RenderPass::ClearDepth& clearDepth : renderPassSubmitInfo.renderPass->GetClearDepth())
	{
		VkClearValue clearValue{};
		clearValue.depthStencil.depth = clearDepth.clearDepth;
		clearValue.depthStencil.stencil = clearDepth.clearStencil;
		vkClearValues.emplace_back(clearValue);
	}

	VkRenderPassBeginInfo info{};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = std::static_pointer_cast<VulkanRenderPass>(renderPassSubmitInfo.renderPass)->GetRenderPass();
	info.framebuffer = std::static_pointer_cast<VulkanFrameBuffer>(renderPassSubmitInfo.frameBuffer)->GetFrameBuffer();
	info.renderArea.extent.width = renderPassSubmitInfo.width;
	info.renderArea.extent.height = renderPassSubmitInfo.height;
	info.clearValueCount = vkClearValues.size();
	info.pClearValues = vkClearValues.data();
	vkCmdBeginRenderPass(frame->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = static_cast<float>(renderPassSubmitInfo.height);
	viewport.width = static_cast<float>(renderPassSubmitInfo.width);
	viewport.height = -static_cast<float>(renderPassSubmitInfo.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	const VkRect2D scissor{ { 0, 0}, { renderPassSubmitInfo.width, renderPassSubmitInfo.height } };
	vkCmdSetViewport(frame->CommandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(frame->CommandBuffer, 0, 1, &scissor);
}

void VulkanRenderer::EndRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo)
{
	const ImGui_ImplVulkanH_Frame* frame = static_cast<ImGui_ImplVulkanH_Frame*>(renderPassSubmitInfo.frame);
	vkCmdEndRenderPass(frame->CommandBuffer);
	Vk::device->CommandEndLabel(frame->CommandBuffer);
}

void VulkanRenderer::BindBuffers(
	const VkCommandBuffer commandBuffer,
	const std::shared_ptr<Buffer>& vertexBuffer,
	const std::shared_ptr<Buffer>& instanceBuffer,
	const std::shared_ptr<Buffer>& indexBuffer,
	const size_t instanceBufferOffset)
{
	constexpr VkDeviceSize vertexOffsets[] = { 0 };

	const VkBuffer vertexBuffers[] = { std::static_pointer_cast<VulkanBuffer>(vertexBuffer)->GetBuffer() };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, vertexOffsets);

	if (instanceBuffer)
	{
		const VkDeviceSize offsetsInstance[] = { instanceBufferOffset };

		const VkBuffer instanceBuffers[] = { std::static_pointer_cast<VulkanBuffer>(instanceBuffer)->GetBuffer() };
		vkCmdBindVertexBuffers(commandBuffer, 1, 1, instanceBuffers, offsetsInstance);
	}

	vkCmdBindIndexBuffer(commandBuffer, std::static_pointer_cast<VulkanBuffer>(indexBuffer)->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void VulkanRenderer::DrawIndexed(const VkCommandBuffer commandBuffer, const uint32_t indexCount, const uint32_t instanceCount)
{
	drawCallsCount++;
	vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
}
#include "VulkanRenderer.h"

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanFrameBuffer.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanComputePipeline.h"
#include "VulkanRenderPass.h"
#include "VulkanUniformWriter.h"
#include "VulkanWindow.h"

using namespace Pengine;
using namespace Vk;

VulkanRenderer::VulkanRenderer()
	: Renderer()
{

}

void VulkanRenderer::Render(
	const std::vector<std::shared_ptr<Buffer>>& vertexBuffers,
	const std::shared_ptr<Buffer>& indexBuffer,
	const int indexCount,
	const std::shared_ptr<Pipeline>& pipeline,
	const std::shared_ptr<Buffer>& instanceBuffer,
	const size_t instanceBufferOffset,
	const size_t count,
	const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
	void* frame)
{
	const ImGui_ImplVulkanH_Frame* vkFrame = static_cast<ImGui_ImplVulkanH_Frame*>(frame);

	std::shared_ptr<VulkanGraphicsPipeline> vkPipeline;
	if (pipeline)
	{
		vkPipeline = std::dynamic_pointer_cast<VulkanGraphicsPipeline>(pipeline);
	}
	else
	{
		return;
	}
	
	vkPipeline->Bind(vkFrame->CommandBuffer);

	std::vector<VkDescriptorSet> descriptorSets;
	for (const auto& uniformWriter : uniformWriters)
	{
		descriptorSets.emplace_back(std::static_pointer_cast<VulkanUniformWriter>(
			uniformWriter)->GetDescriptorSet());
	}

	if (!descriptorSets.empty())
	{
		vkCmdBindDescriptorSets(
			vkFrame->CommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vkPipeline->GetPipelineLayout(),
			0,
			descriptorSets.size(),
			&descriptorSets[0],
			0,
			nullptr);
	}

	vertexCount += (indexCount / 3) * count;
	BindBuffers(vkFrame->CommandBuffer, vertexBuffers, instanceBuffer, indexBuffer, instanceBufferOffset);
	DrawIndexed(vkFrame->CommandBuffer, indexCount, count);
}

void VulkanRenderer::Dispatch(
	const std::shared_ptr<Pipeline>& pipeline,
	const glm::uvec3& groupCount,
	const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
	void* frame)
{
	const ImGui_ImplVulkanH_Frame* vkFrame = static_cast<ImGui_ImplVulkanH_Frame*>(frame);

	std::shared_ptr<VulkanComputePipeline> vkPipeline;
	if (pipeline)
	{
		vkPipeline = std::dynamic_pointer_cast<VulkanComputePipeline>(pipeline);
	}
	else
	{
		return;
	}

	vkPipeline->Bind(vkFrame->CommandBuffer);

	std::vector<VkDescriptorSet> descriptorSets;
	for (const auto& uniformWriter : uniformWriters)
	{
		descriptorSets.emplace_back(std::static_pointer_cast<VulkanUniformWriter>(
			uniformWriter)->GetDescriptorSet());
	}

	if (!descriptorSets.empty())
	{
		vkCmdBindDescriptorSets(
			vkFrame->CommandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			vkPipeline->GetPipelineLayout(),
			0,
			descriptorSets.size(),
			&descriptorSets[0],
			0,
			nullptr);
	}

	vkCmdDispatch(vkFrame->CommandBuffer, groupCount.x, groupCount.y, groupCount.z);
}

void VulkanRenderer::MemoryBarrierFragmentReadWrite(void* frame)
{
	const ImGui_ImplVulkanH_Frame* vkFrame = static_cast<ImGui_ImplVulkanH_Frame*>(frame);

	VkMemoryBarrier memoryBarrier{};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	vkCmdPipelineBarrier(
		vkFrame->CommandBuffer,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		1,
		&memoryBarrier,
		0,
		nullptr,
		0,
		nullptr);
}

void VulkanRenderer::BeginCommandLabel(
	const std::string& name,
	const glm::vec3& color,
	void* frame)
{
	const ImGui_ImplVulkanH_Frame* imGuiFrame = static_cast<ImGui_ImplVulkanH_Frame*>(frame);
	Vk::device->CommandBeginLabel(name, imGuiFrame->CommandBuffer, color);
}

void VulkanRenderer::EndCommandLabel(void* frame)
{
	const ImGui_ImplVulkanH_Frame* imGuiFrame = static_cast<ImGui_ImplVulkanH_Frame*>(frame);
	Vk::device->CommandEndLabel(imGuiFrame->CommandBuffer);
}

void VulkanRenderer::BeginRenderPass(
	const RenderPass::SubmitInfo& renderPassSubmitInfo,
	const std::string& debugName,
	const glm::vec3& debugColor)
{
	const ImGui_ImplVulkanH_Frame* frame = static_cast<ImGui_ImplVulkanH_Frame*>(renderPassSubmitInfo.frame);
 
	if (!debugName.empty())
	{
		BeginCommandLabel(debugName, debugColor, renderPassSubmitInfo.frame);
	}
	else
	{
		BeginCommandLabel(renderPassSubmitInfo.renderPass->GetName(), debugColor, renderPassSubmitInfo.frame);
	}

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

	const glm::ivec2 size = renderPassSubmitInfo.frameBuffer->GetSize();

	VkRenderPassBeginInfo info{};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = std::static_pointer_cast<VulkanRenderPass>(renderPassSubmitInfo.renderPass)->GetRenderPass();
	info.framebuffer = std::static_pointer_cast<VulkanFrameBuffer>(renderPassSubmitInfo.frameBuffer)->GetFrameBuffer();
	info.renderArea.extent.width = size.x;
	info.renderArea.extent.height = size.y;
	info.clearValueCount = vkClearValues.size();
	info.pClearValues = vkClearValues.data();
	vkCmdBeginRenderPass(frame->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = static_cast<float>(size.y);
	viewport.width = static_cast<float>(size.x);
	viewport.height = -static_cast<float>(size.y);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	const VkRect2D scissor{ { 0, 0}, { (uint32_t)size.x, (uint32_t)size.y } };
	vkCmdSetViewport(frame->CommandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(frame->CommandBuffer, 0, 1, &scissor);
}

void VulkanRenderer::EndRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo)
{
	const ImGui_ImplVulkanH_Frame* frame = static_cast<ImGui_ImplVulkanH_Frame*>(renderPassSubmitInfo.frame);
	vkCmdEndRenderPass(frame->CommandBuffer);
	EndCommandLabel(renderPassSubmitInfo.frame);
}

void VulkanRenderer::BindBuffers(
	const VkCommandBuffer commandBuffer,
	const std::vector<std::shared_ptr<Buffer>>& vertexBuffers,
	const std::shared_ptr<Buffer>& instanceBuffer,
	const std::shared_ptr<Buffer>& indexBuffer,
	const size_t instanceBufferOffset)
{
	std::vector<VkBuffer> vkVertexBuffers;
	std::vector<VkDeviceSize> vkVertexOffsets;
	for (const std::shared_ptr<Buffer> vertexBuffer : vertexBuffers)
	{
		vkVertexBuffers.emplace_back(std::static_pointer_cast<VulkanBuffer>(vertexBuffer)->GetBuffer());
		vkVertexOffsets.emplace_back(0);
	}

	if (instanceBuffer)
	{
		vkVertexBuffers.emplace_back(std::static_pointer_cast<VulkanBuffer>(instanceBuffer)->GetBuffer());
		vkVertexOffsets.emplace_back(instanceBufferOffset);
	}

	vkCmdBindVertexBuffers(commandBuffer, 0, vkVertexBuffers.size(), vkVertexBuffers.data(), vkVertexOffsets.data());
	vkCmdBindIndexBuffer(commandBuffer, std::static_pointer_cast<VulkanBuffer>(indexBuffer)->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void VulkanRenderer::DrawIndexed(const VkCommandBuffer commandBuffer, const uint32_t indexCount, const uint32_t instanceCount)
{
	drawCallsCount++;
	vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
}
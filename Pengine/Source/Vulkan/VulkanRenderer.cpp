#include "VulkanRenderer.h"

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanFrameBuffer.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanComputePipeline.h"
#include "VulkanRenderPass.h"
#include "VulkanTexture.h"
#include "VulkanUniformWriter.h"
#include "VulkanWindow.h"

#include "../Core/Profiler.h"

using namespace Pengine;
using namespace Vk;

VulkanRenderer::VulkanRenderer()
	: Renderer()
{

}

void VulkanRenderer::Render(
	const std::vector<std::shared_ptr<Buffer>>& vertexBuffers,
	const std::vector<size_t>& vertexBufferOffsets,
	const std::shared_ptr<Buffer>& indexBuffer,
	const size_t indexBufferOffset,
	const int indexCount,
	const std::shared_ptr<Pipeline>& pipeline,
	const std::shared_ptr<Buffer>& instanceBuffer,
	const size_t instanceBufferOffset,
	const size_t count,
	const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
	void* frame)
{
	PROFILER_SCOPE(__FUNCTION__);

	const VulkanFrameInfo* vkFrame = static_cast<VulkanFrameInfo*>(frame);
	std::shared_ptr<VulkanGraphicsPipeline> vkPipeline = std::static_pointer_cast<VulkanGraphicsPipeline>(pipeline);
	
	if (m_Pipeline != pipeline)
	{
		m_Pipeline = pipeline;
		vkPipeline->Bind(vkFrame->CommandBuffer);
	}

	m_DescriptorSets.clear();
	m_DescriptorSets.reserve(uniformWriters.size());
	for (const auto& uniformWriter : uniformWriters)
	{
		m_DescriptorSets.emplace_back(std::static_pointer_cast<VulkanUniformWriter>(
			uniformWriter)->GetDescriptorSet());
	}

	if (!m_DescriptorSets.empty())
	{
		PROFILER_SCOPE("BindDescriptorSets");

		vkCmdBindDescriptorSets(
			vkFrame->CommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vkPipeline->GetPipelineLayout(),
			0,
			m_DescriptorSets.size(),
			&m_DescriptorSets[0],
			0,
			nullptr);
	}

	vertexCount += (indexCount / 3) * count;
	BindBuffers(vkFrame->CommandBuffer, vertexBuffers, vertexBufferOffsets, instanceBuffer, instanceBufferOffset, indexBuffer, indexBufferOffset);
	DrawIndexed(vkFrame->CommandBuffer, indexCount, count);
}

void VulkanRenderer::Dispatch(
	const std::shared_ptr<Pipeline>& pipeline,
	const glm::uvec3& groupCount,
	const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
	void* frame)
{
	PROFILER_SCOPE(__FUNCTION__);

	const VulkanFrameInfo* vkFrame = static_cast<VulkanFrameInfo*>(frame);

	std::shared_ptr<VulkanComputePipeline> vkPipeline;
	if (pipeline)
	{
		vkPipeline = std::dynamic_pointer_cast<VulkanComputePipeline>(pipeline);
	}
	else
	{
		return;
	}

	if (m_Pipeline != pipeline)
	{
		m_Pipeline = pipeline;
		vkPipeline->Bind(vkFrame->CommandBuffer);
	}

	m_DescriptorSets.clear();
	m_DescriptorSets.reserve(uniformWriters.size());
	for (const auto& uniformWriter : uniformWriters)
	{
		m_DescriptorSets.emplace_back(std::static_pointer_cast<VulkanUniformWriter>(
			uniformWriter)->GetDescriptorSet());
	}

	if (!m_DescriptorSets.empty())
	{
		vkCmdBindDescriptorSets(
			vkFrame->CommandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			vkPipeline->GetPipelineLayout(),
			0,
			m_DescriptorSets.size(),
			&m_DescriptorSets[0],
			0,
			nullptr);
	}

	vkCmdDispatch(vkFrame->CommandBuffer, groupCount.x, groupCount.y, groupCount.z);
}

void VulkanRenderer::MemoryBarrierFragmentReadWrite(void* frame)
{
	PROFILER_SCOPE(__FUNCTION__);

	const VulkanFrameInfo* vkFrame = static_cast<VulkanFrameInfo*>(frame);

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
	PROFILER_SCOPE(__FUNCTION__);
	const VulkanFrameInfo* vkFrame = static_cast<VulkanFrameInfo*>(frame);
	GetVkDevice()->CommandBeginLabel(name, vkFrame->CommandBuffer, color);
}

void VulkanRenderer::EndCommandLabel(void* frame)
{
	PROFILER_SCOPE(__FUNCTION__);
	const VulkanFrameInfo* vkFrame = static_cast<VulkanFrameInfo*>(frame);
	GetVkDevice()->CommandEndLabel(vkFrame->CommandBuffer);
}

void VulkanRenderer::ClearDepthStencilImage(
	std::shared_ptr<Texture> texture,
	const RenderPass::ClearDepth& clearDepth,
	void* frame)
{
	const VulkanFrameInfo* vkFrame = static_cast<VulkanFrameInfo*>(frame);
	const std::shared_ptr<VulkanTexture> vkTexture = std::static_pointer_cast<VulkanTexture>(texture);
	
	VkClearDepthStencilValue clearValue{};
	clearValue.depth = clearDepth.clearDepth;
	clearValue.stencil = clearDepth.clearStencil;
	
	VkImageSubresourceRange range{};
	range.aspectMask = VulkanTexture::ConvertAspectMask(vkTexture->GetAspectMask());
	range.baseMipLevel = 0;
	range.levelCount = vkTexture->GetMipLevels();
	range.baseArrayLayer = 0;
	range.layerCount = vkTexture->GetLayerCount();

	GetVkDevice()->ClearDepthStencilImage(vkTexture->GetImage(), vkTexture->GetLayout(), &clearValue, 1, &range, vkFrame->CommandBuffer);
}

void VulkanRenderer::BeginRenderPass(
	const RenderPass::SubmitInfo& renderPassSubmitInfo,
	const std::string& debugName,
	const glm::vec3& debugColor)
{
	PROFILER_SCOPE(__FUNCTION__);

	const VulkanFrameInfo* frame = static_cast<VulkanFrameInfo*>(renderPassSubmitInfo.frame);

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
	if (renderPassSubmitInfo.viewport)
	{
		viewport.x = renderPassSubmitInfo.viewport->position.x;
		viewport.y = renderPassSubmitInfo.viewport->position.y;
		viewport.width = renderPassSubmitInfo.viewport->size.x;
		viewport.height = renderPassSubmitInfo.viewport->size.y;
		viewport.minDepth = renderPassSubmitInfo.viewport->minMaxDepth.x;
		viewport.maxDepth = renderPassSubmitInfo.viewport->minMaxDepth.y;
	}
	else
	{
		viewport.x = 0;
		viewport.y = static_cast<float>(size.y);
		viewport.width = static_cast<float>(size.x);
		viewport.height = -static_cast<float>(size.y);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
	}

	VkRect2D scissor{};
	if (renderPassSubmitInfo.scissors)
	{
		scissor =
		{
			{ renderPassSubmitInfo.scissors->offset.x, renderPassSubmitInfo.scissors->offset.y },
			{ renderPassSubmitInfo.scissors->size.x, renderPassSubmitInfo.scissors->size.y }
		};
	}
	else
	{
		scissor = { { 0, 0 }, { (uint32_t)size.x, (uint32_t)size.y } };
	}

	vkCmdSetViewport(frame->CommandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(frame->CommandBuffer, 0, 1, &scissor);
}

void VulkanRenderer::EndRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo)
{
	PROFILER_SCOPE(__FUNCTION__);

	const VulkanFrameInfo* frame = static_cast<VulkanFrameInfo*>(renderPassSubmitInfo.frame);
	vkCmdEndRenderPass(frame->CommandBuffer);
	EndCommandLabel(renderPassSubmitInfo.frame);
}

void VulkanRenderer::SetScissors(const RenderPass::Scissors& scissors, void* frame)
{
	PROFILER_SCOPE(__FUNCTION__);

	const VkRect2D scissor =
	{
		{ scissors.offset.x, scissors.offset.y },
		{ scissors.size.x, scissors.size.y }
	};

	const VulkanFrameInfo* vkFrame = static_cast<VulkanFrameInfo*>(frame);
	vkCmdSetScissor(vkFrame->CommandBuffer, 0, 1, &scissor);
}

void VulkanRenderer::SetViewport(const RenderPass::Viewport& viewport, void* frame)
{
	PROFILER_SCOPE(__FUNCTION__);

	VkViewport vkViewport{};
	vkViewport.x = viewport.position.x;
	vkViewport.y = viewport.position.y;
	vkViewport.width = viewport.size.x;
	vkViewport.height =viewport.size.y;
	vkViewport.minDepth = viewport.minMaxDepth.x;
	vkViewport.maxDepth = viewport.minMaxDepth.y;

	const VulkanFrameInfo* vkFrame = static_cast<VulkanFrameInfo*>(frame);
	vkCmdSetViewport(vkFrame->CommandBuffer, 0, 1, &vkViewport);
}

void VulkanRenderer::BindBuffers(
	const VkCommandBuffer commandBuffer,
	const std::vector<std::shared_ptr<Buffer>>& vertexBuffers,
	const std::vector<size_t>& vertexBufferOffsets,
	const std::shared_ptr<Buffer>& instanceBuffer,
	const size_t instanceBufferOffset,
	const std::shared_ptr<Buffer>& indexBuffer,
	const size_t indexBufferOffset)
{
	PROFILER_SCOPE(__FUNCTION__);

	assert(vertexBuffers.size() == vertexBufferOffsets.size());

	m_VertexBuffers.clear();
	m_VertexBuffers.reserve(vertexBuffers.size());
	for (const std::shared_ptr<Buffer> vertexBuffer : vertexBuffers)
	{
		m_VertexBuffers.emplace_back(std::static_pointer_cast<VulkanBuffer>(vertexBuffer)->GetBuffer());
	}

	m_VertexOffsets.clear();
	m_VertexOffsets.reserve(vertexBufferOffsets.size());
	for (const size_t vertexBufferOffset : vertexBufferOffsets)
	{
		m_VertexOffsets.emplace_back(vertexBufferOffset);
	}

	if (instanceBuffer)
	{
		m_VertexBuffers.emplace_back(std::static_pointer_cast<VulkanBuffer>(instanceBuffer)->GetBuffer());
		m_VertexOffsets.emplace_back(instanceBufferOffset);
	}

	vkCmdBindVertexBuffers(commandBuffer, 0, m_VertexBuffers.size(), m_VertexBuffers.data(), m_VertexOffsets.data());
	vkCmdBindIndexBuffer(commandBuffer, std::static_pointer_cast<VulkanBuffer>(indexBuffer)->GetBuffer(), indexBufferOffset, VK_INDEX_TYPE_UINT32);
}

void VulkanRenderer::DrawIndexed(const VkCommandBuffer commandBuffer, const uint32_t indexCount, const uint32_t instanceCount)
{
	PROFILER_SCOPE(__FUNCTION__);

	drawCallsCount++;
	vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
}

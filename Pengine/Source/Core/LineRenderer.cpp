#include "LineRenderer.h"

#include "RenderPassManager.h"
#include "MaterialManager.h"
#include "Scene.h"

#include "../Graphics/Renderer.h"

#define MAX_BATCH_LINE_COUNT 100
#define LINE_VERTEX_COUNT 2
#define MAX_BATCH_LINE_VERTEX_COUNT MAX_BATCH_LINE_COUNT * LINE_VERTEX_COUNT

using namespace Pengine;

void LineRenderer::Render(const RenderPass::RenderCallbackInfo& renderInfo)
{
	if (std::shared_ptr<BaseMaterial> lineBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/Line.basemat"))
	{
		std::shared_ptr<Pipeline> pipeline = lineBaseMaterial->GetPipeline(renderInfo.submitInfo.renderPass->GetType());
		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = RenderPassManager::GetUniformWriters(pipeline, lineBaseMaterial, nullptr, renderInfo);

		for (const auto& uniformWriter : uniformWriters)
		{
			uniformWriter->Flush();

			for (const auto& [location, buffer] : uniformWriter->GetBuffersByLocation())
			{
				buffer->Flush();
			}
		}

		auto render = [this, &uniformWriters, &renderInfo, &pipeline](
			uint32_t& index,
			uint32_t& batchIndex,
			std::vector<glm::vec3>& lineVertices,
			std::vector<uint32_t>& lineIndices)
		{
			if (m_Batches.size() == batchIndex)
			{
				Batch batch;
				batch.m_VertexBuffer = Buffer::Create(
					sizeof(glm::vec3) * 2,
					MAX_BATCH_LINE_COUNT * 2,
					Buffer::Usage::VERTEX_BUFFER,
					Buffer::MemoryType::CPU);

				batch.m_IndexBuffer = Buffer::Create(
					sizeof(uint32_t),
					MAX_BATCH_LINE_COUNT * 2,
					Buffer::Usage::INDEX_BUFFER,
					Buffer::MemoryType::CPU);

				m_Batches.emplace_back(batch);
			}

			Batch& batch = m_Batches[batchIndex];

			batch.m_VertexBuffer->WriteToBuffer(lineVertices.data(), lineVertices.size() * sizeof(glm::vec3));
			batch.m_IndexBuffer->WriteToBuffer(lineIndices.data(), lineIndices.size() * sizeof(uint32_t));

			renderInfo.renderer->Render(
				batch.m_VertexBuffer,
				batch.m_IndexBuffer,
				index,
				pipeline,
				nullptr,
				0,
				1,
				uniformWriters,
				renderInfo.submitInfo);

			batchIndex++;
			index = 0;
			lineVertices.clear();
			lineIndices.clear();
		};

		std::queue<Line>& lines = renderInfo.scene->GetVisualizer().GetLines();
		if (!lines.empty())
		{
			std::vector<glm::vec3> lineVertices;
			std::vector<uint32_t> lineIndices;
			lineVertices.reserve(MAX_BATCH_LINE_VERTEX_COUNT * 2);
			lineIndices.reserve(MAX_BATCH_LINE_VERTEX_COUNT);
			uint32_t index = 0;
			uint32_t batchIndex = 0;
			for (; !lines.empty(); lines.pop())
			{
				if (index == MAX_BATCH_LINE_VERTEX_COUNT)
				{
					render(index, batchIndex, lineVertices, lineIndices);
				}

				Line& line = lines.front();
				lineVertices.emplace_back(line.start);
				lineVertices.emplace_back(line.color);
				lineVertices.emplace_back(line.end);
				lineVertices.emplace_back(line.color);

				lineIndices.emplace_back(index);
				index++;
				lineIndices.emplace_back(index);
				index++;
			}

			render(index, batchIndex, lineVertices, lineIndices);
		}
	}
}

void LineRenderer::ShutDown()
{
	m_Batches.clear();
}

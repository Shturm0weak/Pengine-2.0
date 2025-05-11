#include "UIRenderer.h"

#include "FontManager.h"
#include "RenderPassManager.h"
#include "MaterialManager.h"
#include "TextureManager.h"
#include "Scene.h"
#include "Time.h"

#include "../Graphics/Renderer.h"

#include "../Components/Canvas.h"

// Also need to change in Shaders/RectangleUI.frag.
#define MAX_BATCH_QUAD_COUNT 10000

#define QUAD_VERTEX_COUNT 4
#define QUAD_INDEX_COUNT 6
#define MAX_BATCH_QUAD_VERTEX_COUNT MAX_BATCH_QUAD_COUNT * QUAD_VERTEX_COUNT
#define MAX_BATCH_QUAD_INDEX_COUNT MAX_BATCH_QUAD_COUNT * QUAD_INDEX_COUNT

using namespace Pengine;

UIRenderer::UIRenderer()
{
	m_QuadVertices.reserve(MAX_BATCH_QUAD_VERTEX_COUNT);
	m_QuadIndices.reserve(MAX_BATCH_QUAD_INDEX_COUNT);
	m_WhiteTexture = TextureManager::GetInstance().GetWhite().get();
}

UIRenderer ::~UIRenderer()
{
	m_Batches.clear();
}

void UIRenderer::DrawQuad(
	const glm::vec2& position,
	const glm::vec2& size,
	const glm::vec2& uvMin,
	const glm::vec2& uvMax,
	const glm::vec4& color,
	const glm::vec4& cornerRadius,
	const glm::mat4& projectionMat4,
	void* texture,
	const bool isText)
{
	const glm::vec2 quadHalfSize = size * 0.5f;
	const glm::vec2 quadCenter = position + quadHalfSize;

	const int instanceIndex = m_QuadInstances.size();

	QuadInstance quadInstance{};
	quadInstance.color = color;
	quadInstance.quadCenterAndHalfSize = { quadCenter.x, quadCenter.y, quadHalfSize.x, quadHalfSize.y };
	quadInstance.customData[0] = isText;

	QuadVertex quadVertex{};
	quadVertex.instanceIndex = instanceIndex;

	quadVertex.positionInPixels = position;
	quadVertex.position = projectionMat4 * glm::vec4{ position.x, position.y, 0.0f, 1.0f };
	quadVertex.uv = uvMin;
	quadVertex.cornerRadius = cornerRadius[0];
	m_QuadVertices.emplace_back(quadVertex);

	quadVertex.positionInPixels = position + glm::vec2(size.x, 0.0f);
	quadVertex.position = projectionMat4 * glm::vec4{ position.x + size.x, position.y, 0.0f, 1.0f };
	quadVertex.uv = glm::vec2(uvMax.x, uvMin.y);
	quadVertex.cornerRadius = cornerRadius[1];
	m_QuadVertices.emplace_back(quadVertex);

	quadVertex.positionInPixels = position + glm::vec2(0.0f, size.y);
	quadVertex.position = projectionMat4 * glm::vec4{ position.x, position.y + size.y, 0.0f, 1.0f };
	quadVertex.uv = glm::vec2(uvMin.x, uvMax.y);
	quadVertex.cornerRadius = cornerRadius[2];
	m_QuadVertices.emplace_back(quadVertex);

	quadVertex.positionInPixels = position + glm::vec2(size.x, size.y);
	quadVertex.position = projectionMat4 * glm::vec4{ position.x + size.x, position.y + size.y, 0.0f, 1.0f };
	quadVertex.uv = uvMax;
	quadVertex.cornerRadius = cornerRadius[3];
	m_QuadVertices.emplace_back(quadVertex);

	m_QuadIndices.emplace_back(m_QuadCount * QUAD_VERTEX_COUNT + 0);
	m_QuadIndices.emplace_back(m_QuadCount * QUAD_VERTEX_COUNT + 1);
	m_QuadIndices.emplace_back(m_QuadCount * QUAD_VERTEX_COUNT + 2);
	m_QuadIndices.emplace_back(m_QuadCount * QUAD_VERTEX_COUNT + 2);
	m_QuadIndices.emplace_back(m_QuadCount * QUAD_VERTEX_COUNT + 3);
	m_QuadIndices.emplace_back(m_QuadCount * QUAD_VERTEX_COUNT + 1);

	m_QuadCount++;
	m_CurrentTextureId = texture;

	m_QuadInstances.emplace_back(quadInstance);
}

void UIRenderer::Render(
	const std::vector<entt::entity>& entities,
	std::shared_ptr<BaseMaterial> baseMaterial,
	const RenderPass::RenderCallbackInfo& renderInfo)
{
	// Potential flaws of this renderer:
	// - If color.a < 1.0f the previous draw command will be recorded.
	//   But if next color.a == 1.0f the previous draw command will not be recorded.
	//   Then it may cause some artifacts.
	// - If a group of images has the same texture for all images then the whole group will be drawn in one draw call.
	//   But if the texture is semi transparent and images intersect each other, then it may cause some artifacts.

	if (!baseMaterial)
	{
		Logger::Error("Trying to render UI, but base material is nullptr!");
		return;
	}

	std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderInfo.renderPass->GetName());
	if (!pipeline)
	{
		Logger::Error("Trying to render UI, but pipeline is nullptr!");
		return;
	}

	// TODO: Rework, for now just because we need to clear the ui framebuffer which will be passed to final render pass to compose.
	if (entities.empty())
	{
		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderView->GetFrameBuffer(UI);

		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);
		renderInfo.renderer->EndRenderPass(submitInfo);
	}

	uint32_t batchIndex = 0;
	for (const entt::entity entity : entities)
	{
		Canvas& canvas = renderInfo.scene->GetRegistry().get<Canvas>(entity);

		glm::mat4 projectionMat4 = glm::ortho(0.0f, (float)canvas.size.x, (float)canvas.size.y, 0.0f);

		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;

		if (canvas.drawInMainViewport)
		{
			submitInfo.frameBuffer = renderInfo.renderView->GetFrameBuffer(renderInfo.renderPass->GetName());
		}
		else
		{
			submitInfo.frameBuffer = canvas.frameBuffer;
		}

		if (canvas.commands.length > 0)
		{
			renderInfo.renderer->BeginRenderPass(submitInfo);
		}

		for (int i = 0; i < canvas.commands.length; i++)
		{
			Clay_RenderCommand* renderCommand = &canvas.commands.internalArray[i];

			glm::vec2 position = { renderCommand->boundingBox.x, renderCommand->boundingBox.y };
			const glm::vec2 size = { renderCommand->boundingBox.width, renderCommand->boundingBox.height };
			const glm::vec2 boundingBoxPosition = position;
			const glm::vec2 boundingBoxSize = size;

			glm::vec4 color{};
			glm::vec4 cornerRadius{};
			void* textureId = nullptr;

			switch (renderCommand->commandType)
			{
				case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
				{
					color =
					{
						renderCommand->renderData.rectangle.backgroundColor.r,
						renderCommand->renderData.rectangle.backgroundColor.g,
						renderCommand->renderData.rectangle.backgroundColor.b,
						renderCommand->renderData.rectangle.backgroundColor.a,
					};
					cornerRadius =
					{
						renderCommand->renderData.rectangle.cornerRadius.topLeft,
						renderCommand->renderData.rectangle.cornerRadius.topRight,
						renderCommand->renderData.rectangle.cornerRadius.bottomLeft,
						renderCommand->renderData.rectangle.cornerRadius.bottomRight,
					};

					textureId = m_WhiteTexture;

					if (m_QuadIndices.size() == MAX_BATCH_QUAD_INDEX_COUNT)
					{
						RenderBatch(batchIndex, renderInfo, pipeline);
					}

					if (color.a < 1.0f || m_CurrentTextureId != textureId)
					{
						RecordPreviousDrawCommand(batchIndex, pipeline);
					}

					DrawQuad(position, size, { 0.0f, 1.0f }, { 1.0f, 0.0f }, color, cornerRadius, projectionMat4, textureId, false);

					break;
				}
				case CLAY_RENDER_COMMAND_TYPE_IMAGE:
				{
					color =
					{
						renderCommand->renderData.image.backgroundColor.r,
						renderCommand->renderData.image.backgroundColor.g,
						renderCommand->renderData.image.backgroundColor.b,
						renderCommand->renderData.image.backgroundColor.a,
					};
					cornerRadius =
					{
						renderCommand->renderData.image.cornerRadius.topLeft,
						renderCommand->renderData.image.cornerRadius.topRight,
						renderCommand->renderData.image.cornerRadius.bottomLeft,
						renderCommand->renderData.image.cornerRadius.bottomRight,
					};

					textureId = renderCommand->renderData.image.imageData;

					if (m_QuadIndices.size() == MAX_BATCH_QUAD_INDEX_COUNT)
					{
						RenderBatch(batchIndex, renderInfo, pipeline);
					}

					if (color.a < 1.0f || m_CurrentTextureId != textureId)
					{
						RecordPreviousDrawCommand(batchIndex, pipeline);
					}

					DrawQuad(position, size, { 0.0f, 1.0f }, { 1.0f, 0.0f }, color, cornerRadius, projectionMat4, textureId, false);

					break;
				}
				case CLAY_RENDER_COMMAND_TYPE_TEXT:
				{
					color =
					{
						renderCommand->renderData.text.textColor.r,
						renderCommand->renderData.text.textColor.g,
						renderCommand->renderData.text.textColor.b,
						renderCommand->renderData.text.textColor.a,
					};

					const auto font = FontManager::GetInstance().GetFont(renderCommand->renderData.text.fontId, renderCommand->renderData.text.fontSize);

					textureId = font->atlas.get();

					if (color.a < 1.0f || m_CurrentTextureId != textureId)
					{
						RecordPreviousDrawCommand(batchIndex, pipeline);
					}

					const float lineHeight = renderCommand->boundingBox.height + renderCommand->renderData.text.lineHeight;

					for (int j = 0; j < renderCommand->renderData.text.stringContents.length; ++j)
					{
						const auto& character = renderCommand->renderData.text.stringContents.chars[j];
						const auto& glyph = font->glyphs[character];

						glm::vec2 glyphPosition{};
						glyphPosition.x = position.x + glyph.bearing.x;
						glyphPosition.y = position.y + (glyph.size.y - glyph.bearing.y) + (lineHeight - glyph.size.y);

						if (m_QuadIndices.size() == MAX_BATCH_QUAD_INDEX_COUNT)
						{
							RenderBatch(batchIndex, renderInfo, pipeline);
						}

						DrawQuad(glyphPosition, glyph.size, glyph.uvMin, glyph.uvMax, color, cornerRadius, projectionMat4, textureId, true);

						position.x += glyph.advance + renderCommand->renderData.text.letterSpacing;
					}

					break;
				}
				case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
				{
					RecordPreviousDrawCommand(batchIndex, pipeline);

					RenderPass::Scissors scissors{};
					scissors.size = boundingBoxSize;
					scissors.offset = boundingBoxPosition;
					m_Scissors.emplace(scissors);
					break;
				}
				case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
				{
					RecordPreviousDrawCommand(batchIndex, pipeline);

					m_Scissors = std::nullopt;
					break;
				}
			}
		}

		RenderBatch(batchIndex, renderInfo, pipeline);

		if (canvas.commands.length > 0)
		{
			renderInfo.renderer->EndRenderPass(submitInfo);
		}
	}
}

void UIRenderer::RecordPreviousDrawCommand(const uint32_t batchIndex, std::shared_ptr<Pipeline> pipeline)
{
	Batch::DrawCommand drawCommand{};

	Batch& batch = GetOrCreateBatch(batchIndex, pipeline);
	if (batch.drawCommands.empty())
	{
		if (m_QuadIndices.empty())
		{
			return;
		}

		drawCommand.indexCount = m_QuadIndices.size();
		drawCommand.quadCount = m_QuadCount;
	}
	else
	{
		Batch::DrawCommand lastDrawCommand = batch.drawCommands.back();

		// Trying to add command that has been already added.
		if (lastDrawCommand.quadCount == m_QuadCount)
		{
			return;
		}

		drawCommand.indexCount = (m_QuadCount - lastDrawCommand.quadCount) * QUAD_INDEX_COUNT;
		drawCommand.indexBufferOffset = lastDrawCommand.quadCount * batch.indexBuffer->GetInstanceSize();
		drawCommand.quadCount = m_QuadCount;
	}

	if (m_Scissors)
	{
		drawCommand.scissors = *m_Scissors;
	}

	drawCommand.textureId = m_CurrentTextureId;
	m_CurrentTextureId = nullptr;

	batch.drawCommands.emplace_back(drawCommand);
}

void UIRenderer::RenderBatch(
	uint32_t& batchIndex,
	const RenderPass::RenderCallbackInfo& renderInfo,
	std::shared_ptr<Pipeline> pipeline)
{
	if (m_QuadVertices.empty() || m_QuadIndices.empty() || m_QuadInstances.empty())
	{
		return;
	}

	Batch& batch = GetOrCreateBatch(batchIndex, pipeline);

	batch.uniformBuffer->WriteToBuffer(m_QuadInstances.data(), m_QuadInstances.size() * sizeof(QuadInstance));
	batch.vertexBuffer->WriteToBuffer(m_QuadVertices.data(), m_QuadVertices.size() * sizeof(QuadVertex));
	batch.indexBuffer->WriteToBuffer(m_QuadIndices.data(), m_QuadIndices.size() * sizeof(uint32_t));

	batch.uniformBuffer->Flush();
	batch.vertexBuffer->Flush();
	batch.indexBuffer->Flush();

	batch.uniformWriter->Flush();

	RecordPreviousDrawCommand(batchIndex, pipeline);

	for (const auto& drawCommand : batch.drawCommands)
	{
		Texture* texture = static_cast<Texture*>(drawCommand.textureId ? drawCommand.textureId : m_WhiteTexture);

		if (drawCommand.scissors)
		{
			renderInfo.renderer->SetScissors(*drawCommand.scissors, renderInfo.frame);
		}

		renderInfo.renderer->Render(
			{ batch.vertexBuffer },
			{ 0 },
			batch.indexBuffer,
			drawCommand.indexBufferOffset,
			drawCommand.indexCount,
			pipeline,
			nullptr,
			0,
			1,
			{ texture->GetUniformWriter(), batch.uniformWriter },
			renderInfo.frame);
	}

	batchIndex++;
	m_QuadCount = 0;
	m_QuadVertices.clear();
	m_QuadIndices.clear();
	m_QuadInstances.clear();
	m_CurrentTextureId = nullptr;
	batch.drawCommands.clear();
}

UIRenderer::Batch& UIRenderer::GetOrCreateBatch(const uint32_t batchIndex, std::shared_ptr<Pipeline> pipeline)
{
	if (m_Batches.size() == batchIndex)
	{
		Batch batch;
		batch.vertexBuffer = Buffer::Create(
			sizeof(QuadVertex) * QUAD_VERTEX_COUNT,
			MAX_BATCH_QUAD_COUNT,
			Buffer::Usage::VERTEX_BUFFER,
			MemoryType::CPU,
			true);

		batch.indexBuffer = Buffer::Create(
			sizeof(uint32_t) * QUAD_INDEX_COUNT,
			MAX_BATCH_QUAD_COUNT,
			Buffer::Usage::INDEX_BUFFER,
			MemoryType::CPU,
			true);

		batch.uniformBuffer = Buffer::Create(
			sizeof(QuadInstance),
			MAX_BATCH_QUAD_COUNT,
			Buffer::Usage::STORAGE_BUFFER,
			MemoryType::CPU,
			true);

		batch.uniformWriter = UniformWriter::Create(pipeline->GetUniformLayout(1));
		batch.uniformWriter->WriteBuffer("InstanceBuffer", batch.uniformBuffer);
		batch.uniformWriter->Flush();

		m_Batches.emplace_back(batch);
	}

	return m_Batches[batchIndex];
}

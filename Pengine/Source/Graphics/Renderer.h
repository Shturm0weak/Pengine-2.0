#pragma once

#include "../Core/Core.h"
#include "../Core/GameObject.h"
#include "../Core/TextureSlots.h"
#include "../Graphics/Buffer.h"
#include "../Graphics/FrameBuffer.h"
#include "../Graphics/RenderPass.h"
#include "../Graphics/UniformWriter.h"

namespace Pengine
{

	class PENGINE_API Renderer
	{
	public:
		static std::shared_ptr<Renderer> Create(const glm::ivec2& size);

		Renderer(const glm::ivec2& size);
		~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		void Update(void* frame, std::shared_ptr<class Window> window,
			std::shared_ptr<class Camera> camera);

		std::shared_ptr<FrameBuffer> GetRenderPassFrameBuffer(const std::string& type) const;

		void SetFrameBufferToRenderPass(const std::string& type,
			std::shared_ptr<FrameBuffer> frameBuffer);

		void Resize(const glm::ivec2& size);

		//TODO: rework.
		bool BindTexture(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<BaseMaterial> baseMaterial,
			std::shared_ptr<class Material> material,
			TextureSlots& globalTextureSlots);

		//TODO: rework.
		// Call only before the render call.
		void FlushTextures(std::shared_ptr<BaseMaterial> baseMaterial, std::shared_ptr<Pipeline> pipeline, TextureSlots& globalTextureSlots);

		virtual void Render(std::shared_ptr<Mesh> mesh,
			std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> instanceBuffer,
			size_t instanceBufferOffset, size_t count,
			const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
			RenderPass::SubmitInfo renderPassSubmitInfo) = 0;
	protected:

		virtual void BeginRenderPass(RenderPass::SubmitInfo renderPassSubmitInfo) = 0;

		virtual void EndRenderPass(RenderPass::SubmitInfo renderPassSubmitInfo) = 0;

		std::unordered_map<std::string, std::shared_ptr<FrameBuffer>> m_FrameBuffersByRenderPassType;
	};

}
#pragma once

#include "../Core/Core.h"
#include "../Graphics/Buffer.h"
#include "../Graphics/FrameBuffer.h"
#include "../Graphics/RenderPass.h"
#include "../Graphics/UniformWriter.h"

namespace Pengine
{

	class Mesh;
	class Window;
	class Camera;
	class Scene;
	class Pipeline;
	class Entity;

	class PENGINE_API Renderer : public std::enable_shared_from_this<Renderer>
	{
	public:
		static std::shared_ptr<Renderer> Create(const glm::ivec2& size);

		explicit Renderer();
		virtual ~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) = delete;

		void Update(
			void* frame,
			const std::shared_ptr<Window>& window,
			const std::shared_ptr<Scene>& scene,
			const std::shared_ptr<Entity>& camera);

		std::shared_ptr<FrameBuffer> GetRenderPassFrameBuffer(const std::string& type) const;

		void SetFrameBufferToRenderPass(const std::string& type,
			const std::shared_ptr<FrameBuffer>& frameBuffer);

		void Resize(const glm::ivec2& size) const;

		virtual void Render(
			const std::shared_ptr<Mesh>& mesh,
			const std::shared_ptr<Pipeline>& pipeline,
			const std::shared_ptr<Buffer>& instanceBuffer,
			size_t instanceBufferOffset,
			size_t count,
			const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
			const RenderPass::SubmitInfo& renderPassSubmitInfo) = 0;

	protected:
		virtual void BeginRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo) = 0;

		virtual void EndRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo) = 0;

		std::unordered_map<std::string, std::shared_ptr<FrameBuffer>> m_FrameBuffersByRenderPassType;
	};

}
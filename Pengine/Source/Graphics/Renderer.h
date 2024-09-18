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

		explicit Renderer(const glm::ivec2& size);
		virtual ~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) = delete;

		void Update(
			void* frame,
			const std::shared_ptr<Window>& window,
			const std::shared_ptr<Scene>& scene,
			const std::shared_ptr<Entity>& camera,
			const glm::mat4& projection,
			const glm::ivec2& viewportSize);

		std::shared_ptr<UniformWriter> GetUniformWriter(const std::string& renderPassName) const;

		void SetUniformWriter(const std::string& renderPassName, std::shared_ptr<UniformWriter> uniformWriter);

		std::shared_ptr<Buffer> GetBuffer(const std::string& name) const;

		void SetBuffer(const std::string& name, std::shared_ptr<Buffer> buffer);

		std::shared_ptr<FrameBuffer> GetRenderPassFrameBuffer(const std::string& type) const;

		void SetFrameBufferToRenderPass(const std::string& type,
			const std::shared_ptr<FrameBuffer>& frameBuffer);

		void Resize(const glm::ivec2& size) const;

		virtual void BeginRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo) = 0;

		virtual void EndRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo) = 0;

		virtual void Render(
			const std::shared_ptr<Buffer>& vertices,
			const std::shared_ptr<Buffer>& indices,
			const int indexCount,
			const std::shared_ptr<Pipeline>& pipeline,
			const std::shared_ptr<Buffer>& instanceBuffer,
			size_t instanceBufferOffset,
			size_t count,
			const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
			void* frame) = 0;

	protected:
		std::unordered_map<std::string, std::shared_ptr<FrameBuffer>> m_FrameBuffersByRenderPassType;
		std::unordered_map<std::string, std::shared_ptr<UniformWriter>> m_UniformWriterByRenderPassType;
		std::unordered_map<std::string, std::shared_ptr<Buffer>> m_BuffersByName;
	};

}
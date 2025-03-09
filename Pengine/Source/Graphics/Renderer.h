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

	class PENGINE_API Renderer
	{
	public:
		struct RenderViewportInfo
		{
			std::shared_ptr<Entity> camera;
			std::shared_ptr<RenderTarget> renderTarget;
			glm::mat4 projection;
			glm::ivec2 size;
		};

		static std::shared_ptr<Renderer> Create();

		Renderer() = default;
		virtual ~Renderer() = default;
		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) = delete;

		static void Update(
			void* frame,
			const std::shared_ptr<Window>& window,
			const std::shared_ptr<Renderer>& renderer,
			const std::map<std::shared_ptr<Scene>, std::vector<RenderViewportInfo>>& viewportsByScene);

		virtual void BeginRenderPass(
			const RenderPass::SubmitInfo& renderPassSubmitInfo,
			const std::string& debugName = {},
			const glm::vec3& debugColor = topLevelRenderPassDebugColor) = 0;

		virtual void EndRenderPass(const RenderPass::SubmitInfo& renderPassSubmitInfo) = 0;

		virtual void Render(
			const std::vector<std::shared_ptr<Buffer>>& vertexBuffers,
			const std::shared_ptr<Buffer>& indexBuffer,
			const int indexCount,
			const std::shared_ptr<Pipeline>& pipeline,
			const std::shared_ptr<Buffer>& instanceBuffer,
			size_t instanceBufferOffset,
			size_t count,
			const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
			void* frame) = 0;

		virtual void Dispatch(
			const std::shared_ptr<Pipeline>& pipeline,
			const glm::uvec3& groupCount,
			const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters,
			void* frame) = 0;

		virtual void MemoryBarrierFragmentReadWrite(void* frame) = 0;

		virtual void BeginCommandLabel(
			const std::string& name,
			const glm::vec3& color,
			void* frame) = 0;

		virtual void EndCommandLabel(void* frame) = 0;
	};

}
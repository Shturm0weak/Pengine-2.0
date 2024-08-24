#pragma once

#include "../Core/Core.h"

#include "Texture.h"
#include "Format.h"
#include "UniformWriter.h"

namespace Pengine
{
	const std::string GBuffer = "GBuffer";
	const std::string Deferred = "Deferred";
	const std::string DefaultReflection = "DefaultReflection";
	const std::string Atmosphere = "Atmosphere";
	
	class FrameBuffer;
	class Window;
	class Camera;
	class Renderer;
	class Scene;
	class Entity;

	class PENGINE_API RenderPass
	{
	public:
		struct ClearDepth
		{
			float clearDepth;
			uint32_t clearStencil;
		};

		struct AttachmentDescription
		{
			Texture::Layout layout;
			Format format;
			std::optional<glm::ivec2> size;
			bool isCubeMap = false;
		};

		struct SubmitInfo
		{
			std::shared_ptr<RenderPass> renderPass;
			std::shared_ptr<FrameBuffer> frameBuffer;
			glm::mat4 projection;
			void* frame;
			uint32_t width;
			uint32_t height;
		};

		struct RenderCallbackInfo
		{
			std::shared_ptr<Window> window;
			std::shared_ptr<Renderer> renderer;
			std::shared_ptr<Scene> scene;
			std::shared_ptr<Entity> camera;
			SubmitInfo submitInfo;
		};

		struct CreateInfo
		{
			std::string type;
			std::vector<glm::vec4> clearColors;
			std::vector<ClearDepth> clearDepths;
			std::vector<AttachmentDescription> attachmentDescriptions;
			std::unordered_map<std::string, std::shared_ptr<Buffer>> buffersByName;
			std::function<void(const RenderCallbackInfo&)> renderCallback;
			std::function<void(RenderPass&)> createCallback;
		};

		static std::shared_ptr<RenderPass> Create(const CreateInfo& createInfo);

		explicit RenderPass(const CreateInfo& createInfo);
		virtual ~RenderPass() = default;
		RenderPass(const RenderPass&) = delete;
		RenderPass(RenderPass&&) = delete;
		RenderPass& operator=(const RenderPass&) = delete;
		RenderPass& operator=(RenderPass&&) = delete;

		[[nodiscard]] const std::string& GetType() const { return m_Type; }

		[[nodiscard]] std::vector<glm::vec4> GetClearColors() const { return m_ClearColors; }

		[[nodiscard]] std::vector<ClearDepth> GetClearDepth() const { return m_ClearDepths; }

		[[nodiscard]] std::shared_ptr<UniformWriter> GetUniformWriter() const { return m_UniformWriter; }

		[[nodiscard]] std::shared_ptr<Buffer> GetBuffer(const std::string& name) const;

		void SetBuffer(const std::string& name, const std::shared_ptr<Buffer>& buffer);

		void SetUniformWriter(std::shared_ptr<UniformWriter> uniformWriter);

		void Render(const RenderCallbackInfo& renderInfo) const;

		[[nodiscard]] const std::vector<AttachmentDescription>& GetAttachmentDescriptions() const { return m_AttachmentDescriptions; }

	private:
		std::vector<AttachmentDescription> m_AttachmentDescriptions; 
		std::vector<glm::vec4> m_ClearColors;
		std::vector<ClearDepth> m_ClearDepths;
		std::shared_ptr<UniformWriter> m_UniformWriter;
		std::string m_Type = none;
		std::function<void(RenderCallbackInfo)> m_RenderCallback;
		std::function<void(RenderPass&)> m_CreateCallback;
		std::unordered_map<std::string, std::shared_ptr<Buffer>> m_BuffersByName;
		bool m_IsInitialized = false;

		friend class Renderer;
	};

}
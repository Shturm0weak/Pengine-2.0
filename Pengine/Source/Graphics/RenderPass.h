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
	const std::string Transparent = "Transparent";
	const std::string Final = "Final";
	const std::string SSAO = "SSAO";
	const std::string SSAOBlur = "SSAOBlur";
	const std::string CSM = "CSM";
	
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

		enum class Load
		{
			LOAD = 0,
			CLEAR = 1,
			DONT_CARE = 2,
		};

		enum class Store
		{
			STORE = 0,
			DONT_CARE = 1,
			NONE = 1000301000,
		};

		struct AttachmentDescription
		{
			Texture::SamplerCreateInfo samplerCreateInfo{};
			Texture::Layout layout;
			Format format;
			Load load = Load::CLEAR;
			Store store = Store::STORE;
			std::optional<glm::ivec2> size;
			bool isCubeMap = false;
			uint32_t layercount = 1;
			std::function<std::shared_ptr<FrameBuffer>(Renderer*, uint32_t&)> getFrameBufferCallback;
		};

		struct SubmitInfo
		{
			std::shared_ptr<RenderPass> renderPass;
			std::shared_ptr<FrameBuffer> frameBuffer;
			void* frame;
		};

		struct RenderCallbackInfo
		{
			std::shared_ptr<RenderPass> renderPass;
			std::shared_ptr<Window> window;
			std::shared_ptr<Renderer> renderer;
			std::shared_ptr<Scene> scene;
			std::shared_ptr<Entity> camera;
			glm::mat4 projection;
			glm::ivec2 viewportSize;
			void* frame;
		};

		struct CreateInfo
		{
			std::string type;
			std::vector<glm::vec4> clearColors;
			std::vector<ClearDepth> clearDepths;
			std::vector<AttachmentDescription> attachmentDescriptions;
			std::function<void(const RenderCallbackInfo&)> renderCallback;
			std::function<void(RenderPass&)> createCallback;
			bool resizeWithViewport = false;
			bool createFrameBuffer = true;
			glm::vec2 resizeViewportScale = { 1.0f, 1.0f };
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

		[[nodiscard]] std::vector<AttachmentDescription>& GetAttachmentDescriptions() { return m_AttachmentDescriptions; }

		[[nodiscard]] const bool GetResizeWithViewport() const { return m_ResizeWithViewport; }

		[[nodiscard]] const glm::vec2& GetResizeViewportScale() const { return m_ResizeViewportScale; }

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
		bool m_ResizeWithViewport = false;
		bool m_CreateFrameBuffer = true;
		glm::vec2 m_ResizeViewportScale = { 1.0f, 1.0f };

		friend class Renderer;
	};

}
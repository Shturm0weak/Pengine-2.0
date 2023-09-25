#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

#include "Texture.h"
#include "UniformWriter.h"
#include "Vertex.h"

namespace Pengine
{
	const std::string GBuffer = "GBuffer";
	const std::string Deferred = "Deferred";
	
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
			glm::ivec2 size = { 1, 1 };
			Texture::Layout layout;
			Texture::Format format;
		};

		struct SubmitInfo
		{
			std::shared_ptr<RenderPass> renderPass;
			std::shared_ptr<class FrameBuffer> frameBuffer;
			void* frame;
			uint32_t width;
			uint32_t height;
		};

		struct RenderCallbackInfo
		{
			class Renderer* renderer;
			std::shared_ptr<class Window> window;
			std::shared_ptr<class Camera> camera;
			SubmitInfo submitInfo;
		};

		struct CreateInfo
		{
			std::string type;
			std::vector<glm::vec4> clearColors;
			std::vector<ClearDepth> clearDepths;
			std::vector<AttachmentDescription> attachmentDescriptions;
			std::vector<Vertex::AttributeDescription> attributeDescriptions;
			std::vector<Vertex::BindingDescription> bindingDescriptions;
			std::unordered_map<uint32_t, UniformLayout::Binding> uniformBindings;
			std::unordered_map<std::string, std::shared_ptr<Buffer>> buffersByName;
			std::function<void(RenderCallbackInfo)> renderCallback;
			std::function<void(RenderPass& renderPass)> createCallback;
		};

		static std::shared_ptr<RenderPass> Create(const CreateInfo& createInfo);

		RenderPass(const CreateInfo& createInfo);
		virtual ~RenderPass() = default;
		RenderPass(const RenderPass&) = delete;
		RenderPass& operator=(const RenderPass&) = delete;

		const std::string& GetType() const { return m_Type; }

		std::vector<glm::vec4> GetClearColors() const { return m_ClearColors; }

		std::vector<ClearDepth> GetClearDepth() const { return m_ClearDepths; }

		std::shared_ptr<UniformWriter> GetUniformWriter() const { return m_UniformWriter; }

		std::shared_ptr<Buffer> GetBuffer(const std::string& name) const;

		void SetBuffer(const std::string& name, std::shared_ptr<Buffer> buffer);

		void Render(RenderCallbackInfo renderInfo);

		const std::vector<Vertex::AttributeDescription>& GetAttributeDescriptions() const { return m_AttributeDescriptions;  }

		const std::vector<Vertex::BindingDescription>& GetBindingDescriptions() const { return m_BindingDescriptions; }

		const std::vector<AttachmentDescription>& GetAttachmentDescriptions() const { return m_AttachmentDescriptions; }

	private:
		std::vector<Vertex::AttributeDescription> m_AttributeDescriptions;
		std::vector<Vertex::BindingDescription> m_BindingDescriptions;
		std::vector<AttachmentDescription> m_AttachmentDescriptions; 
		std::vector<glm::vec4> m_ClearColors;
		std::vector<ClearDepth> m_ClearDepths;
		std::shared_ptr<UniformWriter> m_UniformWriter;
		std::string m_Type = none;
		std::function<void(RenderCallbackInfo)> m_RenderCallback;
		std::function<void(RenderPass& renderPass)> m_CreateCallback;
		std::unordered_map<std::string, std::shared_ptr<Buffer>> m_BuffersByName;
	};

}
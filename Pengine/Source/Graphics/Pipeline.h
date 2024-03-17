#pragma once

#include "../Core/Core.h"

#include "RenderPass.h"
#include "UniformLayout.h"

namespace Pengine
{

	class PENGINE_API Pipeline
	{
	public:
		enum class CullMode
		{
			NONE,
			FRONT,
			BACK,
			FRONT_AND_BACK,
		};

		enum class PolygonMode
		{
			FILL,
			LINE
		};

		enum class ShaderType
		{
			VERTEX,
			FRAGMENT
		};

		struct BlendStateAttachment
		{
			// TODO: fill this struct!!!
			bool blendEnabled = false;
		};

		struct CreateInfo
		{
			std::unordered_map<uint32_t, UniformLayout::Binding> uniformBindings;
			std::unordered_map<uint32_t, UniformLayout::Binding> childUniformBindings;
			std::vector<BlendStateAttachment> colorBlendStateAttachments;
			std::shared_ptr<RenderPass> renderPass;
			std::string vertexFilepath;
			std::string fragmentFilepath;
			CullMode cullMode;
			PolygonMode polygonMode;
			bool depthTest;
			bool depthWrite;
		};

		static std::shared_ptr<Pipeline> Create(const CreateInfo& pipelineCreateInfo);

		explicit Pipeline(const CreateInfo& pipelineCreateInfo);
		virtual ~Pipeline() = default;
		Pipeline(const Pipeline&) = delete;
		Pipeline(Pipeline&&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;
		Pipeline& operator=(Pipeline&&) = delete;

		[[nodiscard]] std::shared_ptr<UniformWriter> GetUniformWriter() const { return m_UniformWriter; }

		[[nodiscard]] std::shared_ptr<UniformLayout> GetChildUniformLayout() const { return m_ChildUniformLayout; }

		[[nodiscard]] std::shared_ptr<Buffer> GetBuffer(const std::string& name) const;

		CreateInfo createInfo;

	protected:
		static std::string ReadFile(const std::string & filepath);

		std::unordered_map<std::string, std::shared_ptr<Buffer>> m_BuffersByName;

		std::shared_ptr<UniformWriter> m_UniformWriter;
		std::shared_ptr<UniformLayout> m_ChildUniformLayout;
	};

}
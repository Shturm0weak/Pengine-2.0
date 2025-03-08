#pragma once

#include "../Core/Core.h"
#include "../Core/CustomData.h"
#include "../Graphics/Buffer.h"
#include "../Graphics/FrameBuffer.h"
#include "../Graphics/UniformWriter.h"

namespace Pengine
{
	class PENGINE_API RenderTarget
	{
	public:
		static std::shared_ptr<RenderTarget> Create(
			const std::vector<std::string>& renderPassOrder,
			const glm::ivec2& size);

		explicit RenderTarget(
			const std::vector<std::string>& renderPassOrder,
			const glm::ivec2& size);
		virtual ~RenderTarget();
		RenderTarget(const RenderTarget&) = delete;
		RenderTarget(RenderTarget&&) = delete;
		RenderTarget& operator=(const RenderTarget&) = delete;
		RenderTarget& operator=(RenderTarget&&) = delete;

		std::shared_ptr<UniformWriter> GetUniformWriter(const std::string& name) const;

		void SetUniformWriter(const std::string& name, std::shared_ptr<UniformWriter> uniformWriter);

		void DeleteUniformWriter(const std::string& name) { m_UniformWriterByName.erase(name); }

		std::shared_ptr<Buffer> GetBuffer(const std::string& name) const;

		void SetBuffer(const std::string& name, std::shared_ptr<Buffer> buffer);

		void DeleteBuffer(const std::string& name) { m_BuffersByName.erase(name); }

		std::shared_ptr<FrameBuffer> GetFrameBuffer(const std::string& type) const;

		void SetFrameBuffer(const std::string& name, const std::shared_ptr<FrameBuffer>& frameBuffer);

		void DeleteFrameBuffer(const std::string& name);

		CustomData* GetCustomData(const std::string& name);

		void SetCustomData(const std::string& name, CustomData* data);

		void DeleteCustomData(const std::string& name);

		void Resize(const glm::ivec2& size) const;

	protected:
		std::unordered_map<std::string, std::shared_ptr<FrameBuffer>> m_FrameBuffersByName;
		std::unordered_map<std::string, std::shared_ptr<UniformWriter>> m_UniformWriterByName;
		std::unordered_map<std::string, std::shared_ptr<Buffer>> m_BuffersByName;
		std::unordered_map<std::string, CustomData*> m_CustomDataByName;

		std::vector<std::string> m_RenderPassOrder;
	};

}

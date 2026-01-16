#pragma once

#include "../Core/Core.h"
#include "../Core/CustomData.h"
#include "../Graphics/Buffer.h"
#include "../Graphics/FrameBuffer.h"
#include "../Graphics/UniformWriter.h"

namespace Pengine
{

	/**
	 * RenderView is supposed to store GPU data like buffers, frame buffers, etc.
	 * It's an abstraction that makes resource management a little easier.
	 * For example, we want to have a collection of resources that belong to a camera or a scene,
	 * so we can just add a RenderView to them and that's it, now we can use it there.
	 */
	class PENGINE_API RenderView
	{
	public:
		static std::shared_ptr<RenderView> Create(
			const std::vector<std::string>& renderPassOrder,
			const glm::ivec2& size);

		explicit RenderView(
			const std::vector<std::string>& renderPassOrder,
			const glm::ivec2& size);
		virtual ~RenderView();
		RenderView(const RenderView&) = delete;
		RenderView(RenderView&&) = delete;
		RenderView& operator=(const RenderView&) = delete;
		RenderView& operator=(RenderView&&) = delete;

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

		std::shared_ptr<Texture> GetStorageImage(const std::string& name);

		void SetStorageImage(const std::string& name, std::shared_ptr<Texture> texture);

		void DeleteStorageImage(const std::string& name) { m_StorageImagesByName.erase(name); }

		void Resize(const glm::ivec2& size);

	protected:
		std::unordered_map<std::string, std::shared_ptr<FrameBuffer>> m_FrameBuffersByName;
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_StorageImagesByName;
		std::unordered_map<std::string, std::shared_ptr<UniformWriter>> m_UniformWriterByName;
		std::unordered_map<std::string, std::shared_ptr<Buffer>> m_BuffersByName;
		std::unordered_map<std::string, CustomData*> m_CustomDataByName;

		std::vector<std::string> m_RenderPassOrder;
	};

}

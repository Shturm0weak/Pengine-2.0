#pragma once

#include "../Core/Core.h"

#include "Buffer.h"
#include "Texture.h"
#include "UniformLayout.h"

namespace Pengine
{

	class PENGINE_API UniformWriter
	{
	public:
		static std::shared_ptr<UniformWriter> Create(std::shared_ptr<UniformLayout> layout);

		UniformWriter(std::shared_ptr<UniformLayout> layout);
		virtual ~UniformWriter() = default;
		UniformWriter(const UniformWriter&) = delete;
		UniformWriter& operator=(const UniformWriter&) = delete;

		virtual void WriteBuffer(uint32_t location, std::shared_ptr<Buffer> buffer, size_t size = -1, size_t offset = 0) = 0;
		virtual void WriteTexture(uint32_t location, std::shared_ptr<Texture> texture) = 0;
		virtual void WriteTextures(uint32_t location, std::vector<std::shared_ptr<Texture>> textures) = 0;
		virtual void WriteBuffer(const std::string& name, std::shared_ptr<Buffer> buffer, size_t size = -1, size_t offset = 0) = 0;
		virtual void WriteTexture(const std::string& name, std::shared_ptr<Texture> texture) = 0;
		virtual void WriteTextures(const std::string& name, std::vector<std::shared_ptr<Texture>> textures) = 0;
		virtual void Flush() = 0;

		std::shared_ptr<UniformLayout> GetLayout() const { return m_Layout; }

	protected:
		std::shared_ptr<UniformLayout> m_Layout;
	};

}
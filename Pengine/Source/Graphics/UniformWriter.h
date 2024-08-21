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
		static std::shared_ptr<UniformWriter> Create(std::shared_ptr<UniformLayout> uniformLayout);

		explicit UniformWriter(std::shared_ptr<UniformLayout> uniformLayout);
		virtual ~UniformWriter() = default;
		UniformWriter(const UniformWriter&) = delete;
		UniformWriter& operator=(const UniformWriter&) = delete;

		virtual void WriteBuffer(uint32_t location, const std::shared_ptr<Buffer>& buffer, size_t size = -1, size_t offset = 0) = 0;
		virtual void WriteTexture(uint32_t location, const std::shared_ptr<Texture>& texture) = 0;
		virtual void WriteTextures(uint32_t location, const std::vector<std::shared_ptr<Texture>>& textures) = 0;
		virtual void WriteBuffer(const std::string& name, const std::shared_ptr<Buffer>& buffer, size_t size = -1, size_t offset = 0) = 0;
		virtual void WriteTexture(const std::string& name, const std::shared_ptr<Texture>& texture) = 0;
		virtual void WriteTextures(const std::string& name, const std::vector<std::shared_ptr<Texture>>& textures) = 0;
		virtual void Flush() = 0;

		const std::unordered_map<uint32_t, std::shared_ptr<Buffer>>& GetBuffersByLocation() const { return m_BuffersByLocation; }

		std::shared_ptr<Texture> GetTexture(const std::string& name);

	protected:
		std::shared_ptr<UniformLayout> m_UniformLayout;
		std::unordered_map<uint32_t, std::shared_ptr<Buffer>> m_BuffersByLocation;
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_TexturesByName;
	};

}
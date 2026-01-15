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
		static std::shared_ptr<UniformWriter> Create(
			std::shared_ptr<UniformLayout> uniformLayout,
			bool isMultiBuffered = true);

		explicit UniformWriter(
			std::shared_ptr<UniformLayout> uniformLayout,
			bool isMultiBuffered);
		virtual ~UniformWriter();
		UniformWriter(const UniformWriter&) = delete;
		UniformWriter& operator=(const UniformWriter&) = delete;

		void WriteBuffer(uint32_t location, const std::shared_ptr<Buffer>& buffer, size_t size = -1, size_t offset = 0);
		void WriteTexture(uint32_t location, const std::shared_ptr<Texture>& texture, uint32_t dstArrayElement = 0);
		void WriteTextures(uint32_t location, const std::vector<std::shared_ptr<Texture>>& textures, uint32_t dstArrayElement = 0);
		void WriteBuffer(const std::string& name, const std::shared_ptr<Buffer>& buffer, size_t size = -1, size_t offset = 0);
		void WriteTexture(const std::string& name, const std::shared_ptr<Texture>& texture, uint32_t dstArrayElement = 0);
		void WriteTextures(const std::string& name, const std::vector<std::shared_ptr<Texture>>& textures, uint32_t dstArrayElement = 0);
		virtual void Flush() = 0;
		virtual NativeHandle GetNativeHandle() const = 0;

		const std::unordered_map<std::string, std::vector<std::shared_ptr<Buffer>>>& GetBuffersByName() const { return m_BuffersByName; }

		std::vector<std::shared_ptr<Buffer>> GetBuffer(const std::string& name);

		const std::unordered_map<std::string, std::vector<std::shared_ptr<Texture>>>& GetTexturesByName() const { return m_TexturesByName; }

		std::vector<std::shared_ptr<Texture>> GetTexture(const std::string& name);
		
		std::shared_ptr<UniformLayout> GetUniformLayout() const { return m_UniformLayout; }

		[[nodiscard]] bool IsMultiBuffered() const { return m_IsMultiBuffered; }

	protected:
		std::shared_ptr<UniformLayout> m_UniformLayout;

		std::unordered_map<std::string, std::vector<std::shared_ptr<Buffer>>> m_BuffersByName;
		std::unordered_map<std::string, std::vector<std::shared_ptr<Texture>>> m_TexturesByName;

		std::unordered_map<uint32_t, std::string> m_BufferNameByLocation;
		std::unordered_map<uint32_t, std::string> m_TextureNameByLocation;

		struct BufferWrite
		{
			ShaderReflection::ReflectDescriptorSetBinding binding;
			std::vector<std::shared_ptr<Buffer>> buffers;
			size_t offset = 0;
			size_t size = -1;
		};

		struct TextureWrite
		{
			ShaderReflection::ReflectDescriptorSetBinding binding;
			std::vector<std::shared_ptr<Texture>> textures;
			uint32_t dstArrayElement = 0;
		};

		struct Write
		{
			std::unordered_map<uint32_t, BufferWrite> bufferWritesByLocation;
			std::unordered_map<uint32_t, std::vector<TextureWrite>> textureWritesByLocation;
		};

		std::vector<Write> m_Writes;

		std::mutex mutex;

		bool m_IsMultiBuffered = false;
	};

}

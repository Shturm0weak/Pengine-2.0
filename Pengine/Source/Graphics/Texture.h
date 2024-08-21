#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

#include "Format.h"

namespace Pengine
{

	class PENGINE_API Texture : public Asset
	{
	public:
		
		enum class AspectMask
		{
			COLOR,
			DEPTH
		};

		enum class Layout
		{
			UNDEFINED,
			SHADER_READ_ONLY_OPTIMAL,
			COLOR_ATTACHMENT_OPTIMAL,
			DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};

		enum class Usage
		{
			SAMPLED,
			TRANSFER_DST,
			TRANSFER_SRC,
			DEPTH_STENCIL_ATTACHMENT,
			COLOR_ATTACHMENT
		};

		struct CreateInfo
		{
			glm::ivec2 size = { 0, 0 };
			std::vector<uint8_t> data;
			int channels = 0;
			uint32_t mipLevels = 1;
			std::string name;
			std::filesystem::path filepath;
			Format format;
			AspectMask aspectMask;
			std::vector<Usage> usage;
		};

		static std::shared_ptr<Texture> Create(const CreateInfo& textureCreateInfo);

		static std::shared_ptr<Texture> Load(const std::filesystem::path& filepath);
		
		explicit Texture(const CreateInfo& textureCreateInfo);
		virtual ~Texture() = default;
		Texture(const Texture&) = delete;
		Texture(Texture&&) = delete;
		Texture& operator=(const Texture&) = delete;
		Texture& operator=(Texture&&) = delete;

		[[nodiscard]] virtual void* GetId() const { return nullptr;  }

		virtual void GenerateMipMaps() = 0;

		[[nodiscard]] glm::ivec2 GetSize() const { return m_Size; }

		[[nodiscard]] int GetChannels() const { return m_Channels; }

		[[nodiscard]] std::vector<uint8_t> GetData() const { return m_Data; }

		[[nodiscard]] Format GetFormat() const { return m_Format; }

		[[nodiscard]] AspectMask GetAspectMask() const { return m_AspectMask; }

		[[nodiscard]] uint32_t GetMipLevels() const { return m_MipLevels; }

	protected:
		glm::ivec2 m_Size = { 0, 0 };

		int m_Channels = 0;
		uint32_t m_MipLevels = 1;

		std::vector<uint8_t> m_Data;

		Format m_Format{};
		AspectMask m_AspectMask{};
	};

}
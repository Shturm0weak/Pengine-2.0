#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

namespace Pengine
{

	class PENGINE_API Texture : public Asset
	{
	public:
		enum class Format
		{
			R8G8B8A8_SRGB,
			B8G8R8A8_UNORM,
			D32_SFLOAT,
			D32_SFLOAT_S8_UINT,
			D24_UNORM_S8_UINT,
			R32_SFLOAT,
			R32G32_SFLOAT,
			R32G32B32_SFLOAT,
			R32G32B32A32_SFLOAT,
			R16_SFLOAT,
			R16G16_SFLOAT,
			R16G16B16_SFLOAT,
			R16G16B16A16_SFLOAT
		};

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
			uint8_t* data = nullptr;
			int channels = 0;
			uint32_t mipLevels = 1;
			std::string name;
			std::string filepath;
			Format format;
			AspectMask aspectMask;
			std::vector<Usage> usage;
		};

		static std::shared_ptr<Texture> Create(CreateInfo textureCreateInfo);

		static std::shared_ptr<Texture> Load(const std::string& filepath);
		
		Texture(CreateInfo textureCreateInfo);
		virtual ~Texture() = default;
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		virtual void* GetId() const { return nullptr;  }

		virtual void GenerateMipMaps() = 0;

		glm::ivec2 GetSize() const { return m_Size; }

		int GetChannels() const { return m_Channels; }

		uint8_t* GetData() const { return m_Data; }

		Format GetFormat() const { return m_Format; }

		AspectMask GetAspectMask() const { return m_AspectMask; }

		uint32_t GetMipLevels() const { return m_MipLevels; }

	protected:
		glm::ivec2 m_Size = { 0, 0 };

		int m_Channels = 0;
		uint32_t m_MipLevels = 1;

		uint8_t* m_Data;

		Format m_Format;
		AspectMask m_AspectMask;
	};

}
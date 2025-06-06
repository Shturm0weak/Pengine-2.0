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
			COLOR_ATTACHMENT,
			STORAGE
		};

		struct SamplerCreateInfo
		{
			enum class Filter
			{
				NEAREST = 0,
				LINEAR = 1
			};

			enum class AddressMode
			{
				REPEAT = 0,
				MIRRORED_REPEAT = 1,
				CLAMP_TO_EDGE = 2,
				CLAMP_TO_BORDER = 3,
				MIRROR_CLAMP_TO_EDGE = 4
			};

			enum class MipmapMode
			{
				MODE_NEAREST = 0,
				MODE_LINEAR = 1
			};

			enum class BorderColor
			{
				FLOAT_TRANSPARENT_BLACK = 0,
				INT_TRANSPARENT_BLACK = 1,
				FLOAT_OPAQUE_BLACK = 2,
				INT_OPAQUE_BLACK = 3,
				FLOAT_OPAQUE_WHITE = 4,
				INT_OPAQUE_WHITE = 5
			};

			float maxAnisotropy = 16.0f;
			Filter filter = Filter::LINEAR;
			AddressMode addressMode = AddressMode::REPEAT;
			MipmapMode mipmapMode = MipmapMode::MODE_LINEAR;
			BorderColor borderColor = BorderColor::FLOAT_OPAQUE_BLACK;
		};

		struct Region
		{
			glm::ivec3 srcOffset = { 0, 0, 0 };
			glm::ivec3 dstOffset = { 0, 0, 0 };
			glm::uvec3 extent = { 0, 0, 1 };
		};

		struct Meta
		{
			std::filesystem::path filepath;
			UUID uuid;
			bool createMipMaps = true;
			bool srgb = true;
		};

		struct CreateInfo
		{
			SamplerCreateInfo samplerCreateInfo{};
			glm::ivec2 size = { 0, 0 };
			void* data = nullptr;
			uint32_t instanceSize = sizeof(uint8_t);
			uint32_t mipLevels = 1;
			uint32_t layerCount = 1;
			int channels = 0;
			std::string name;
			std::filesystem::path filepath;
			Format format;
			AspectMask aspectMask;
			std::vector<Usage> usage;
			bool isCubeMap = false;
			bool isMultiBuffered = false;
			
			MemoryType memoryType = MemoryType::GPU;

			Meta meta;
		};

		struct LoadInfo
		{
			std::filesystem::path filepath;
			bool createMipMaps = true;
			bool srgb = true;
		};

		struct SubresourceLayout
		{
			uint64_t offset;
			uint64_t size;
			uint64_t rowPitch;
			uint64_t arrayPitch;
			uint64_t depthPitch;
		};

		static std::shared_ptr<Texture> Create(const CreateInfo& createInfo);

		static std::shared_ptr<Texture> Load(const std::filesystem::path& filepath, bool flip, const Texture::Meta& meta);
		
		explicit Texture(const CreateInfo& createInfo);
		virtual ~Texture() = default;
		Texture(const Texture&) = delete;
		Texture(Texture&&) = delete;
		Texture& operator=(const Texture&) = delete;
		Texture& operator=(Texture&&) = delete;

		[[nodiscard]] virtual void* GetId() const { return nullptr;  }

		[[nodiscard]] virtual void* GetData() const = 0;

		[[nodiscard]] virtual SubresourceLayout GetSubresourceLayout() const = 0;

		virtual void GenerateMipMaps(void* frame = nullptr) = 0;

		virtual void Copy(std::shared_ptr<Texture> src, const Region& region, void* frame = nullptr) = 0;

		[[nodiscard]] glm::ivec2 GetSize() const { return m_Size; }

		[[nodiscard]] int GetChannels() const { return m_Channels; }

		[[nodiscard]] Format GetFormat() const { return m_Format; }

		[[nodiscard]] AspectMask GetAspectMask() const { return m_AspectMask; }

		[[nodiscard]] uint32_t GetMipLevels() const { return m_MipLevels; }

		[[nodiscard]] uint32_t GetLayerCount() const { return m_LayerCount; }

		[[nodiscard]] MemoryType GetMemoryType() const { return m_MemoryType; }

		[[nodiscard]] Meta GetMeta() const { return m_Meta; }

		[[nodiscard]] bool IsMultiBuffered() const { return m_IsMultiBuffered; }

		[[nodiscard]] std::shared_ptr<class UniformWriter> GetUniformWriter() const { return m_UniformWriter; }

	protected:
		glm::ivec2 m_Size = { 0, 0 };

		int m_Channels = 0;
		uint32_t m_MipLevels = 1;
		uint32_t m_LayerCount = 1;
		uint32_t m_InstanceSize = sizeof(uint8_t);

		Format m_Format{};
		AspectMask m_AspectMask{};
		MemoryType m_MemoryType{};

		bool m_IsMultiBuffered = false;

		Meta m_Meta{};

		std::shared_ptr<class UniformWriter> m_UniformWriter;
	};

}
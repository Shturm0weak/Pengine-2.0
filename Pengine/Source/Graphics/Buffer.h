#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API Buffer
	{
	public:
		enum class Usage
		{
			STORAGE_BUFFER,
			UNIFORM_BUFFER,
			VERTEX_BUFFER,
			INDEX_BUFFER,
			TRANSFER_SRC,
			TRANSFER_DST
		};

		static std::shared_ptr<Buffer> Create(
			size_t instanceSize,
			uint32_t instanceCount, std::vector<Usage> usage);

		Buffer() = default;
		virtual ~Buffer() = default;
		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;

		virtual void Map(size_t size = -1, size_t offset = 0) = 0;

		virtual void Unmap() = 0;

		virtual void* GetData() = 0;

		virtual void WriteToBuffer(void* data, size_t size = -1,
			size_t offset = 0) = 0;

		virtual void Copy(std::shared_ptr<Buffer> buffer) = 0;

		virtual uint32_t GetInstanceCount() const = 0;

		virtual size_t GetInstanceSize() const = 0;
	};

}
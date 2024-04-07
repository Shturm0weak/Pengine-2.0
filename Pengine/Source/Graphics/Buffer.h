#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API Buffer
	{
	public:
		enum class Usage
		{
			UNIFORM_BUFFER,
			VERTEX_BUFFER,
			INDEX_BUFFER
		};

		enum class MemoryType
		{
			CPU,
			GPU
		};

		static std::shared_ptr<Buffer> Create(
			size_t instanceSize,
			uint32_t instanceCount,
			Usage usage,
			MemoryType memoryType);

		Buffer(MemoryType memoryType);
		virtual ~Buffer() = default;
		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;

		[[nodiscard]] virtual void* GetData() const = 0;

		virtual void WriteToBuffer(
			void* data,
			size_t size,
			size_t offset = 0) = 0;

		virtual void Copy(
			const std::shared_ptr<Buffer>& buffer,
			size_t dstOffset) = 0;

		virtual void Flush() = 0;

		[[nodiscard]] virtual size_t GetSize() const = 0;

		[[nodiscard]] virtual uint32_t GetInstanceCount() const = 0;

		[[nodiscard]] virtual size_t GetInstanceSize() const = 0;

		[[nodiscard]] MemoryType GetMemoryType() const { return m_MemoryType; }

	protected:
		MemoryType m_MemoryType = MemoryType::CPU;
	};

}
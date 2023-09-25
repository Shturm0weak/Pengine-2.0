#pragma once

#include "../Core/Core.h"
#include "../Graphics/Buffer.h"

namespace Pengine
{

	class PENGINE_API SrBuffer : public Buffer
	{
	public:
		SrBuffer(size_t size);
		~SrBuffer();
		SrBuffer(const SrBuffer&) = delete;
		SrBuffer& operator=(const SrBuffer&) = delete;

		virtual void WriteToBuffer(void* data, size_t size = -1,
			size_t offset = 0) override;

		virtual void Copy(std::shared_ptr<Buffer> buffer) override {};

	private:
		uint8_t* m_Data = nullptr;
		size_t m_Size = 0;
	};

}
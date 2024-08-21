#pragma once

#include "../Core/Core.h"
#include "../Core/Logger.h"

#include "../Graphics/Buffer.h"

namespace Pengine
{

	class PENGINE_API WriterBufferHelper
	{
	public:
		template<typename T>
		static void WriteToBuffer(
			class BaseMaterial* baseMaterial,
			std::shared_ptr<Buffer> buffer,
			const std::string& uniformBufferName,
			const std::string& valueName,
			T& value)
		{
			uint32_t size, offset;

			const bool found = baseMaterial->GetUniformDetails(uniformBufferName, valueName, size, offset);

			if (buffer && found)
			{
				if (sizeof(T) != size)
				{
					Logger::Warning("Failed to write to buffer: " + uniformBufferName + " | " + valueName + ", size is different!");
				}

				buffer->WriteToBuffer((void*)&value, size, offset);
			}
			else
			{
				Logger::Warning("Failed to write to buffer: " + uniformBufferName + " | " + valueName + "!");
			}
		}
	};

}
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

		template<typename T>
		static T GetBufferValue(
			class BaseMaterial* baseMaterial,
			std::shared_ptr<Buffer> buffer,
			const std::string& uniformBufferName,
			const std::string& valueName)
		{
			uint32_t size, offset;

			const bool found = baseMaterial->GetUniformDetails(uniformBufferName, valueName, size, offset);

			if (buffer && found)
			{
				if (sizeof(T) != size)
				{
					Logger::Warning("Failed to get buffer value: " + uniformBufferName + " | " + valueName + ", size is different!");
				}

				return *(T*)(((uint8_t*)buffer->GetData()) + offset);
			}
			else
			{
				Logger::Warning("Failed to get buffer value: " + uniformBufferName + " | " + valueName + "!");
			}
		}
	};

}
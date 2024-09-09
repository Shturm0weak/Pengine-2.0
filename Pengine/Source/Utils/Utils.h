#pragma once

#include "../Core/Core.h"
#include "../Core/Logger.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_decompose.hpp"

#include <filesystem>
#include <fstream>

namespace Pengine::Utils
{
	inline glm::vec3 GetPosition(const glm::mat4& position)
	{
		const float* matPtr = glm::value_ptr(position);
		return { matPtr[12], matPtr[13], matPtr[14] };
	}

	inline glm::vec3 GetScale(const glm::mat4& scale)
	{
		const float* matPtr = glm::value_ptr(scale);
		return { matPtr[0], matPtr[5], matPtr[10] };
	}

	inline bool Contains(const std::string& string, const std::string& content)
	{
		return string.find(content) != std::string::npos;
	}

	inline std::string GetFileFormat(const std::filesystem::path& filepath)
	{
		if (filepath.has_extension())
		{
			return filepath.extension().string();
		}

		return {};
	}

	inline std::string GetFilename(const std::filesystem::path& filepath)
	{
		return filepath.filename().replace_extension().string();
	}

	inline std::filesystem::path EraseFileFormat(const std::filesystem::path& filepath)
	{
		if (filepath.has_extension())
		{
			return std::filesystem::path(filepath).replace_extension("").string();
		}

		return {};
	}

	template<typename T, typename U>
	T Find(const U& key, const std::unordered_map<U, T>& map)
	{
		auto foundItem = map.find(key);
		if (foundItem != map.end())
		{
			return foundItem->second;
		}

		return {};
	}

	template<typename T, typename U>
	T Find(const U& key, const std::map<U, T>& map)
	{
		auto foundItem = map.find(key);
		if (foundItem != map.end())
		{
			return foundItem->second;
		}

		return {};
	}

	template<typename T>
	bool Find(const T& key, const std::vector<T>& vector)
	{
		return std::find(vector.begin(), vector.end(), key) != vector.end();
	}

	template<typename T>
	bool Erase(std::vector<T>& vector, const T& item)
	{
		auto foundItem = std::find(vector.begin(), vector.end(), item);
		if (foundItem != vector.end())
		{
			vector.erase(foundItem);
			return true;
		}

		return false;
	}

	inline std::string Erase(std::string string, const std::string& what)
	{
		if (const size_t index = string.find(what); index != std::string::npos)
		{
			return string.erase(index, index + what.size());
		}

	 	return string;
	}

	inline std::wstring Erase(std::wstring string, const std::wstring& what)
	{
		if (const size_t index = string.find(what); index != std::wstring::npos)
		{
			return string.erase(index, index + what.size());
		}

		return string;
	}

	inline size_t StringTypeToSize(const std::string& type)
	{
		if (type == "int")
		{
			return 4;
		}
		if (type == "sampler")
		{
			return 4;
		}
		if(type == "float")
		{
			return 4;
		}
		if (type == "vec2")
		{
			return 8;
		}
		if (type == "vec3")
		{
			return 12;
		}
		if (type == "vec4")
		{
			return 16;
		}
		if (type == "color")
		{
			return 16;
		}
		if (type == "mat3")
		{
			return 36;
		}
		if (type == "mat4")
		{
			return 64;
		}

		return -1;
	}

	template<typename T>
	void SetValue(void* data, const size_t offset, T& value)
	{
		*(T*)((char*)data + offset) = value;
	}

	template<typename T>
	T& GetValue(void* data, const size_t offset)
	{
		return *(T*)((char*)data + offset);
	}

	inline std::string Replace(const std::string& string, const char what, const char to)
	{
		std::string replacedString = string;
		std::replace(replacedString.begin(), replacedString.end(), what, to);
		return replacedString;
	}

	inline std::string FindUuid(const std::filesystem::path& filepath)
	{
		auto foundItem = uuidByFilepath.find(filepath);
		if (foundItem != uuidByFilepath.end())
		{
			return foundItem->second;
		}

		return {};
	}

	inline std::filesystem::path FindFilepath(const std::string& uuid)
	{
		auto foundItem = filepathByUuid.find(uuid);
		if (foundItem != filepathByUuid.end())
		{
			return foundItem->second;
		}

		return {};
	}

	inline std::string EraseFromBack(std::string string, const char what)
	{
		if (const size_t index = string.find_last_of(what); index != std::string::npos)
		{
			return string.substr(0, index);
		}

		return string;
	}

	inline std::string EraseFromFront(std::string string, const char what)
	{
		if (const size_t index = string.find_first_of(what); index != std::string::npos)
		{
			return string.substr(index + 1, string.size() - 1);
		}

		return string;
	}

	inline std::string GetShortFilepath(std::filesystem::path filepath)
	{
		return Erase(filepath.string(), std::filesystem::current_path().string() + "\\");
	}

	inline std::string ReadFile(const std::filesystem::path& filepath)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			FATAL_ERROR("Failed to open file: " + filepath.string());
		}

		const size_t fileSize = file.tellg();

		std::string buffer;
		buffer.resize(fileSize);

		file.seekg(0);
		file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
		file.close();

		return buffer;
	}

	inline bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
	{
		// From glm::decompose in matrix_decompose.inl

		using namespace glm;
		using T = float;

		mat4 LocalMatrix(transform);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>())) return false;

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
			LocalMatrix[3][3] = static_cast<T>(1);
		}

		// Next take care of translation (easy).
		translation = vec3(LocalMatrix[3]);
		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

		vec3 Row[3]/*, Pdum3*/;

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		scale.x = length(Row[0]);
		Row[0] = detail::scale(Row[0], static_cast<T>(1));
		scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<T>(1));
		scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

		rotation.y = asin(-Row[0][2]);
		if (cos(rotation.y) != 0)
		{
			rotation.x = atan2(Row[1][2], Row[2][2]);
			rotation.z = atan2(Row[0][1], Row[0][0]);
		}
		else
		{
			rotation.x = atan2(-Row[2][0], Row[1][1]);
			rotation.z = 0;
		}
		return true;
	}

	inline bool DecomposeRotation(const glm::mat4& rotationMat4, glm::vec3& rotation)
	{
		// From glm::decompose in matrix_decompose.inl

		using namespace glm;
		using T = float;

		mat4 LocalMatrix(rotationMat4);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>())) return false;

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
			LocalMatrix[3][3] = static_cast<T>(1);
		}

		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

		vec3 Row[3]/*, Pdum3*/;

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		Row[0] = detail::scale(Row[0], static_cast<T>(1));
		Row[1] = detail::scale(Row[1], static_cast<T>(1));
		Row[2] = detail::scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

		rotation.y = asin(-Row[0][2]);
		if (cos(rotation.y) != 0)
		{
			rotation.x = atan2(Row[1][2], Row[2][2]);
			rotation.z = atan2(Row[0][1], Row[0][0]);
		}
		else
		{
			rotation.x = atan2(-Row[2][0], Row[1][1]);
			rotation.z = 0;
		}
		return true;
	}

}
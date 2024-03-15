#pragma once

#include "../Core/Core.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_decompose.hpp"

#include <filesystem>

namespace Pengine
{

	namespace Utils
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

		inline std::wstring StringToWString(const std::string& str)
		{
			std::wstring wstr;
			size_t size;
			wstr.resize(str.size());
			mbstowcs_s(&size, &wstr[0], wstr.size() + 1, str.c_str(), str.size());
			return wstr;
		}

		inline std::string WStringToString(const std::wstring& wstr)
		{
			std::string str;
			size_t size;
			str.resize(wstr.size());
			wcstombs_s(&size, &str[0], str.size() + 1, wstr.c_str(), wstr.size());
			return str;
		}

		inline bool Contains(const std::string& string, const std::string& content)
		{
			return string.find(content) != std::string::npos;
		}

		inline std::string GetFileFormat(const std::string& filepath)
		{
			size_t index = filepath.find_last_of(".");
			if (index == std::string::npos)
			{
				return {};
			}

			return filepath.substr(index, filepath.size() - 1);
		}

		inline std::string EraseFileFormat(const std::string& filepath)
		{
			size_t index = filepath.find_last_of(".");
			if (index == std::string::npos)
			{
				return {};
			}

			return filepath.substr(0, index);
		}

		template<typename T>
		inline T Find(const std::string& name, std::unordered_map<std::string, T> map)
		{
			auto iter = map.find(name);
			if (iter != map.end())
			{
				return iter->second;
			}

			return {};
		}

		template<typename T>
		inline T Find(const std::wstring& name, std::unordered_map<std::wstring, T> map)
		{
			auto iter = map.find(name);
			if (iter != map.end())
			{
				return iter->second;
			}

			return {};
		}

		template<typename T, typename U>
		inline T Find(const U& key, std::unordered_map<U, T> map)
		{
			auto iter = map.find(key);
			if (iter != map.end())
			{
				return iter->second;
			}

			return {};
		}

		template<typename T>
		inline bool Find(T key, std::vector<T> vector)
		{
			return std::find(vector.begin(), vector.end(), key) != vector.end();
		}

		template<typename T>
		inline bool Erase(std::vector<T>& vector, T item)
		{
			auto foundItem = std::find(vector.begin(), vector.end(), item);
			if (foundItem != vector.end())
			{
				vector.erase(foundItem);
				return true;
			}

			return false;
		}

		inline size_t StringTypeToSize(const std::string& type)
		{
			if (type == "int")
			{
				return 4;
			}
			else if (type == "sampler")
			{
				return 4;
			}
			else if(type == "float")
			{
				return 4;
			}
			else if (type == "vec2")
			{
				return 8;
			}
			else if (type == "vec3")
			{
				return 12;
			}
			else if (type == "vec4")
			{
				return 16;
			}
			else if (type == "color")
			{
				return 16;
			}
			else if (type == "mat3")
			{
				return 36;
			}
			else if (type == "mat4")
			{
				return 64;
			}
		}

		template<typename T>
		inline void SetValue(void* data, size_t offset, T& value)
		{
			*(T*)((char*)data + offset) = value;
		}

		template<typename T>
		inline T& GetValue(void* data, size_t offset)
		{
			return *(T*)((char*)data + offset);
		}

		inline std::string Replace(const std::string& string, char what, char to)
		{
			std::string replacedString = string;
			std::replace(replacedString.begin(), replacedString.end(), what, to);
			return replacedString;
		}

		inline std::string Erase(std::string string, std::string what)
		{
			what = Replace(what, '\\', '/');

			std::string::size_type index = string.find(what);

			if (index != std::string::npos)
			{
				string.erase(index, what.size());
			}

			return string;
		}

		inline std::string EraseDirectoryFromFilePath(const std::string& path)
		{
			size_t slash = path.find_last_of('/');
			if (slash == std::string::npos)
			{
				slash = path.find_last_of('\\');
			}

			return path.substr(slash + 1, path.size() - slash);
		}

		inline std::string ExtractDirectoryFromFilePath(const std::string& path)
		{
			size_t slash = path.find_last_of('/');
			if (slash == std::string::npos)
			{
				slash = path.find_last_of('\\');
			}

			return path.substr(0, slash);
		}

		inline std::string FindUuid(const std::string& filepath)
		{
			return Utils::Find(filepath, uuidByFilepath);
		}

		inline std::string EraseFromString(std::string string, const std::string& what)
		{
			const size_t index = string.find(what);
			if (index != std::string::npos)
			{
				return string.erase(index, index + what.size());
			}

			return string;
		}

		inline std::string EraseFromBack(std::string string, char what)
		{
			const size_t index = string.find_last_of(what);
			if (index != std::string::npos)
			{
				return string.substr(0, index);
			}

			return string;
		}

		inline std::string EraseFromFront(std::string string, char what)
		{
			const size_t index = string.find_first_of(what);
			if (index != std::string::npos)
			{
				return string.substr(index + 1, string.size() - 1);
			}

			return string;
		}

		inline std::string GetShortFilepath(std::string filepath)
		{
			std::string currentFilepath = std::filesystem::current_path().string();
			currentFilepath = Replace(currentFilepath, '\\', '/') + "/";
			filepath = Replace(filepath, '\\', '/');

			return EraseFromString(filepath, currentFilepath);
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

			vec3 Row[3], Pdum3;

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

			vec3 Row[3], Pdum3;

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

	//	PENGINE_API inline int DeleteDirectory(const std::wstring& refcstrRootDirectory,
	//		bool              bDeleteSubdirectories = false)
	//	{
	//		bool            bSubdirectory = false;       // Flag, indicating whether
	//													 // subdirectories have been found
	//		HANDLE          hFile;                       // Handle to directory
	//		std::wstring     strFilePath;                // Filepath
	//		std::wstring     strPattern;                 // Pattern
	//		WIN32_FIND_DATA FileInformation;             // File information


	//		strPattern = refcstrRootDirectory + L"\\*.*";
	//		hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
	//		if (hFile != INVALID_HANDLE_VALUE)
	//		{
	//			do
	//			{
	//				if (FileInformation.cFileName[0] != '.')
	//				{
	//					strFilePath.erase();
	//					strFilePath = refcstrRootDirectory + L"\\" + FileInformation.cFileName;

	//					if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	//					{
	//						if (bDeleteSubdirectories)
	//						{
	//							// Delete subdirectory
	//							int iRC = DeleteDirectory(strFilePath, bDeleteSubdirectories);
	//							if (iRC)
	//								return iRC;
	//						}
	//						else
	//							bSubdirectory = true;
	//					}
	//					else
	//					{
	//						// Set file attributes
	//						if (::SetFileAttributes(strFilePath.c_str(),
	//							FILE_ATTRIBUTE_NORMAL) == FALSE)
	//							return ::GetLastError();

	//						// Delete file
	//						if (::DeleteFile(strFilePath.c_str()) == FALSE)
	//							return ::GetLastError();
	//					}
	//				}
	//			} while (::FindNextFile(hFile, &FileInformation) == TRUE);

	//			// Close handle
	//			::FindClose(hFile);

	//			DWORD dwError = ::GetLastError();
	//			if (dwError != ERROR_NO_MORE_FILES)
	//				return dwError;
	//			else
	//			{
	//				if (!bSubdirectory)
	//				{
	//					// Set directory attributes
	//					if (::SetFileAttributes(refcstrRootDirectory.c_str(),
	//						FILE_ATTRIBUTE_NORMAL) == FALSE)
	//						return ::GetLastError();

	//					// Delete directory
	//					if (::RemoveDirectory(refcstrRootDirectory.c_str()) == FALSE)
	//						return ::GetLastError();
	//				}
	//			}
	//		}

	//		return 0;
	//	}
	}

}
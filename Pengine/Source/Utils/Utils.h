#pragma once

#include "../Core/Core.h"

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
			wstr.resize(str.length());
			mbstowcs_s(&size, &wstr[0], wstr.size() + 1, str.c_str(), str.size());
			return wstr;
		}

		inline std::string WStringToString(const std::wstring& wstr)
		{
			std::string str;
			size_t size;
			str.resize(wstr.length());
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

			return filepath.substr(index + 1, filepath.length() - 1);
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
				string.erase(index, what.length());
			}

			return string;
		}

		inline std::string RemoveDirectoryFromFilePath(const std::string& path)
		{
			size_t slash = path.find_last_of('/');
			if (slash == std::string::npos)
			{
				slash = path.find_last_of('\\');
			}

			return path.substr(slash + 1, path.length() - slash);
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

		inline std::string GetFullFilepath(const std::string& filepath)
		{
			std::string fullFilepath = filepath;
			fullFilepath = Utils::Replace(fullFilepath, '/', '\\');
			if (!Contains(fullFilepath, std::filesystem::current_path().string()))
			{
				fullFilepath = std::filesystem::current_path().string() + "\\" + fullFilepath;
				return fullFilepath;
			}
			else
			{
				return fullFilepath;
			}
		}

		inline std::string FindUuid(std::string filepath)
		{
			return Utils::Find(GetFullFilepath(filepath), uuidByFilepath);
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

		inline std::string GetShortFilepath(const std::string& filepath)
		{
			std::string shortFilepath = filepath;
			if (Contains(shortFilepath, std::filesystem::current_path().string()))
			{
				shortFilepath = Utils::EraseFromFront(shortFilepath, '\\');
				shortFilepath = Utils::EraseFromFront(shortFilepath, '\\');
				shortFilepath = Utils::EraseFromFront(shortFilepath, '\\');
				shortFilepath = Utils::Replace(shortFilepath, '\\', '/');
			}

			return shortFilepath;
		}

	}

}
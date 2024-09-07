#pragma once

#include "Core.h"

#include <filesystem>

namespace Pengine
{

	class PENGINE_API Asset
	{
	public:
		Asset(std::string name, std::filesystem::path filepath)
			: m_Name(std::move(name))
			, m_Filepath(std::move(filepath))
		{}

		Asset(const Asset& asset)
			: m_Name(asset.GetName())
			, m_Filepath(asset.GetFilepath())
		{}

		Asset(Asset&& asset) noexcept
			: m_Name(std::move(asset.m_Name))
			, m_Filepath(std::move(asset.m_Filepath))
		{}

		Asset& operator=(const Asset& asset)
		{
			m_Name = asset.GetName();
			m_Filepath = asset.GetFilepath();

			return *this;
		}

		Asset& operator=(Asset&& asset) noexcept
		{
			m_Name = std::move(asset.m_Name);
			m_Filepath = std::move(asset.m_Filepath);

			return *this;
		}

		[[nodiscard]] const std::string& GetName() const { return m_Name; }

		[[nodiscard]] const std::filesystem::path& GetFilepath() const { return m_Filepath; }

	protected:
		std::string m_Name = none;
		std::filesystem::path m_Filepath = none;
	};

}

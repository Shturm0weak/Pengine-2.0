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

		[[nodiscard]] const std::string& GetName() const { return m_Name; }

		[[nodiscard]] const std::filesystem::path& GetFilepath() const { return m_Filepath; }

	protected:
		std::string m_Name = none;
		std::filesystem::path m_Filepath = none;
	};

}

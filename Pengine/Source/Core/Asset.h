#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API Asset
	{
	public:
		Asset(std::string name, std::string filepath)
			: m_Name(std::move(name))
			, m_Filepath(std::move(filepath))
		{}

		Asset(const Asset& asset)
			: m_Name(asset.GetName())
			, m_Filepath(asset.GetFilepath())
		{}

		[[nodiscard]] const std::string& GetName() const { return m_Name; }

		[[nodiscard]] const std::string& GetFilepath() const { return m_Filepath; }

	protected:
		std::string m_Name = none;
		std::string m_Filepath = none;
	};

}

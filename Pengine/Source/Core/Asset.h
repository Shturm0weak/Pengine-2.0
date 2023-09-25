#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API Asset
	{
	public:
		Asset(const std::string& name, const std::string& filepath)
			: m_Name(name)
			, m_Filepath(filepath)
		{
		}

		const std::string& GetName() const { return m_Name; }

		const std::string& GetFilepath() const { return m_Filepath; }

	protected:
		std::string m_Name = none;
		std::string m_Filepath = none;
	};

}
#include <gtest/gtest.h>

#include "Utils/Utils.h"

using namespace Pengine;

TEST(Utils, GetShortFilepath)
{
	try
	{
		{
			const std::filesystem::path subFilepath = std::filesystem::path("Editor") / "Images" / "FileIcon.png";
			const std::filesystem::path filepath = std::filesystem::current_path() / subFilepath;
			EXPECT_TRUE(Utils::GetShortFilepath(filepath) == subFilepath);
		}
		
		{
			const std::filesystem::path filepath = std::filesystem::path("Editor") / "Images" / "FileIcon.png";
			EXPECT_TRUE(Utils::GetShortFilepath(filepath) == filepath);
		}
	}
	catch (const std::exception& e)
	{
		Logger::Error(e.what());
		FAIL();
	}
}

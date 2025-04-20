#include <gtest/gtest.h>

#include "Core/UUID.h"
#include "Core/Logger.h"

using namespace Pengine;

TEST(UUID, UUID)
{
	try
	{
		UUID uuidDefault;
		std::string uuidString = uuidDefault.ToString();
		UUID uuid = UUID::FromString(uuidString);
		EXPECT_TRUE(uuidDefault == uuid);
	}
	catch (const std::exception& e)
	{
		Logger::Error(e.what());
		FAIL();
	}
}

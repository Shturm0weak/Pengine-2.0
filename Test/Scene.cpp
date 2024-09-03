#include <gtest/gtest.h>

#include "Core/SceneManager.h"
#include "Core/Logger.h"

using namespace Pengine;

TEST(Scene, Create)
{
	try
	{
		std::shared_ptr<Scene> scene = SceneManager::GetInstance().Create("Scene", "Main");
		EXPECT_TRUE(scene);
		if (scene)
		{
			EXPECT_TRUE(scene->GetName() == "Scene");
			EXPECT_TRUE(scene->GetTag() == "Main");
			EXPECT_TRUE(scene->GetEntities().size() == 0);
		}
		EXPECT_TRUE(SceneManager::GetInstance().GetScenesCount() == 1);
		SceneManager::GetInstance().Delete(scene);
		EXPECT_TRUE(SceneManager::GetInstance().GetScenesCount() == 0);
	}
	catch (const std::exception& e)
	{
		Logger::Error(e.what());
		FAIL();
	}
}

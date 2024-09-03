#include <gtest/gtest.h>

#include "Core/SceneManager.h"
#include "Core/Logger.h"

using namespace Pengine;

TEST(Entity, Create)
{
	try
	{
		std::shared_ptr<Scene> scene = SceneManager::GetInstance().Create("Scene", "Main");
		std::shared_ptr<Entity> entity = scene->CreateEntity("GameObject");
		EXPECT_TRUE(entity);
		if (entity)
		{
			EXPECT_TRUE(entity->GetName() == "GameObject");
			EXPECT_TRUE(entity->GetHandle() != entt::tombstone);
			EXPECT_TRUE(entity->GetScene() == scene);
			EXPECT_TRUE(entity->GetParent() == nullptr);
			EXPECT_TRUE(!entity->GetUUID().Get().empty());
		}

		EXPECT_TRUE(!scene->GetEntities().empty());
		scene->DeleteEntity(entity);
		EXPECT_TRUE(scene->GetEntities().empty());

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

TEST(Entity, AddChild)
{
	try
	{
		std::shared_ptr<Scene> scene = SceneManager::GetInstance().Create("Scene", "Main");
		std::shared_ptr<Entity> entity = scene->CreateEntity("GameObject");
		std::shared_ptr<Entity> child = scene->CreateEntity("Child");
		EXPECT_TRUE(entity);
		if (entity)
		{
			entity->AddChild(child);

			EXPECT_TRUE(entity->HasAsChild(child));
			EXPECT_TRUE(child->HasAsParent(entity));

			EXPECT_TRUE(entity == child->GetParent());
			EXPECT_TRUE(!entity->GetChilds().empty());
			entity->RemoveChild(child);
			EXPECT_TRUE(entity->GetChilds().empty());
			EXPECT_TRUE(child->GetParent() == nullptr);
		}

		EXPECT_TRUE(!scene->GetEntities().empty());
		scene->DeleteEntity(entity);
		scene->DeleteEntity(child);
		EXPECT_TRUE(scene->GetEntities().empty());

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
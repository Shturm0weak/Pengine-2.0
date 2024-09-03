#include <gtest/gtest.h>

#include "Core/SceneManager.h"
#include "Components/Transform.h"
#include "Core/Logger.h"

using namespace Pengine;

TEST(Transform, Create)
{
	try
	{
		std::shared_ptr<Scene> scene = SceneManager::GetInstance().Create("Scene", "Main");
		std::shared_ptr<Entity> entity = scene->CreateEntity("GameObject");
		EXPECT_TRUE(entity);
		if (entity)
		{
			Transform& transform = entity->AddComponent<Transform>(entity);

			EXPECT_TRUE(entity->HasComponent<Transform>());

			EXPECT_TRUE(transform.GetPosition() == glm::vec3(0.0f));
			EXPECT_TRUE(transform.GetRotation() == glm::vec3(0.0f));
			EXPECT_TRUE(transform.GetScale() == glm::vec3(1.0f));

			EXPECT_TRUE(transform.GetForward() == glm::vec3(0.0f, 0.0f, -1.0f));

			transform.Translate(glm::vec3(3.0f, 2.0f, 1.0f));
			transform.Rotate(glm::vec3(90.0f, 0.0f, 0.0f));
			transform.Scale(glm::vec3(3.0f, 2.0f, 1.0f));

			EXPECT_TRUE(transform.GetPosition() == glm::vec3(3.0f, 2.0f, 1.0f));
			EXPECT_TRUE(transform.GetRotation() == glm::vec3(90.0f, 0.0f, 0.0f));
			EXPECT_TRUE(transform.GetScale() == glm::vec3(3.0f, 2.0f, 1.0f));

			entity->RemoveComponent<Transform>();
			
			EXPECT_TRUE(!entity->HasComponent<Transform>());
		}

		SceneManager::GetInstance().Delete(scene);
	}
	catch (const std::exception& e)
	{
		Logger::Error(e.what());
		FAIL();
	}
}

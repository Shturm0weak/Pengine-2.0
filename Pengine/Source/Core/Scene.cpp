#include "Scene.h"

#include "Logger.h"
#include "Viewport.h"

using namespace Pengine;

void Scene::Copy(const Scene& scene)
{
	m_Name = scene.GetName();
	m_Filepath = scene.GetFilepath();

	for (const auto& gameObject : scene.m_GameObjects)
	{
		if (gameObject->GetOwner() == nullptr)
		{
			GameObject* createdGameObject = CreateGameObject(gameObject->GetName(),
				gameObject->m_Transform, gameObject->GetUUID());
			createdGameObject->Copy(*gameObject);
		}
	}
}

Scene::Scene(const std::string& name, const std::string& filepath)
	: Asset(name, filepath)
{

}

Scene::Scene(const Scene& scene)
	: Asset(scene.GetName(), scene.GetFilepath())
{
	Copy(scene);
}

void Scene::Clear()
{
	m_Name = none;
	m_Filepath = none;

	while (m_GameObjects.size() > 0)
	{
		DeleteGameObject(m_GameObjects.front());
	}
}

void Scene::operator=(const Scene& scene)
{
	Copy(scene);
}

GameObjectC& Scene::CreateGameObjectC(const std::string& name, const UUID& uuid)
{
	Entity& entity = CreateEntity();
	entity.AddComponent<Transform>();
	return entity.AddComponent<GameObjectC>(entity, name, uuid);
}

GameObject* Scene::CreateGameObject(const std::string& name, const Transform& transform, const UUID& uuid)
{
	const entt::entity entity = m_Registry.create();
	GameObject* gameObject = &m_Registry.emplace<GameObject>(entity, entity, this, name, transform, uuid);

	m_GameObjectsByUUID.emplace(gameObject->GetUUID(), gameObject);
	m_GameObjects.push_back(gameObject);

	return gameObject;
}

Entity Scene::CreateEntity()
{
	return *m_Entities.emplace((this)).first;
}

void Scene::DeleteEntity(Entity& entity)
{
	if (entity.m_Handle != entt::tombstone)
	{
		m_Registry.destroy(entity.m_Handle);
		entity.m_Handle = entt::tombstone;
	}
	m_Entities.erase(entity);
}

void Scene::DeleteGameObject(GameObject* gameObject)
{
	m_Registry.destroy(gameObject->GetEntity());
}

void Scene::DeleteGameObjectLater(GameObject* gameObject)
{
	DeleteGameObject(gameObject);
}

GameObject* Scene::FindGameObjectByName(const std::string& name)
{
	auto gameObject = std::find_if(m_GameObjects.begin(), m_GameObjects.end(), [name](GameObject* gameObject) {
		return gameObject->GetName() == name;
		});

	if (gameObject != m_GameObjects.end())
	{
		return *gameObject;
	}

	return nullptr;
}

std::vector<GameObject*> Scene::FindGameObjects(const std::string& name)
{
	std::vector<GameObject*> gameObjects;
	for (auto gameObject : m_GameObjects)
	{
		if (gameObject->GetName() == name)
		{
			gameObjects.push_back(gameObject);
		}
	}
	return gameObjects;
}

GameObject* Scene::FindGameObjectByUUID(const std::string& uuid)
{
	auto gameObject = m_GameObjectsByUUID.find(uuid);
	if (gameObject != m_GameObjectsByUUID.end())
	{
		return gameObject->second;
	}

	return nullptr;
}

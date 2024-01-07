#include "Scene.h"

#include "Logger.h"
#include "Viewport.h"

using namespace Pengine;

void Scene::Copy(const Scene& scene)
{
	m_Name = scene.GetName();
	m_Filepath = scene.GetFilepath();

	//for (const auto& gameObject : scene.m_GameObjects)
	//{
	//	if (gameObject->GetOwner() == nullptr)
	//	{
	//		GameObject* createdGameObject = CreateGameObject(gameObject->GetName(),
	//			gameObject->m_Transform, gameObject->GetUUID());
	//		createdGameObject->Copy(*gameObject);
	//	}
	//}
}

Scene::Scene(const std::string& name, const std::string& filepath)
	: Asset(name, filepath)
{

}

Scene::~Scene()
{
	Clear();
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

	m_Registry.clear();
	m_Entities.clear();
}

void Scene::operator=(const Scene& scene)
{
	Copy(scene);
}

std::shared_ptr<Entity> Scene::CreateEntity(const std::string& name, const UUID& uuid)
{
	std::shared_ptr<Entity> entity = std::make_shared<Entity>(shared_from_this(), name, uuid);
	m_Entities.emplace_back(entity);

	return entity;
}

void Scene::DeleteEntity(std::shared_ptr<Entity> entity)
{
	if (entity->GetHandle() != entt::tombstone)
	{
		m_Registry.destroy(entity->GetHandle());
	}
	
	auto entityToErase = std::find(m_Entities.begin(), m_Entities.end(), entity);
	if (entityToErase != m_Entities.end())
	{
		m_Entities.erase(entityToErase);
	}

	entity = nullptr;
}

std::shared_ptr<Entity> Scene::FindEntityByUUID(const std::string& uuid)
{
	for (std::shared_ptr<Entity> entity : m_Entities)
	{
		if (entity->GetUUID().Get() == uuid)
		{
			return entity;
		}
	}

	return nullptr;
}

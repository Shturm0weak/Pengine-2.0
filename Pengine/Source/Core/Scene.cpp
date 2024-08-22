#include "Scene.h"

#include "Logger.h"
#include "Viewport.h"

#include "../Components/Transform.h"
#include "../Components/Camera.h"

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

Scene::Scene(const std::string& name, const std::filesystem::path& filepath)
	: Asset(name, filepath)
{

}

Scene::~Scene()
{
	Clear();
}

Scene::Scene(const Scene& scene)
	: Asset(scene.GetName(), scene.GetFilepath())
	, enable_shared_from_this(scene)
{
	Copy(scene);
}

void Scene::Clear()
{
	m_Name = none;
	m_Filepath = none;

	m_Entities.clear();
	m_Registry.clear();
}

Scene& Scene::operator=(const Scene& scene)
{
	if(this != &scene)
	{
		Copy(scene);
	}

	return *this;
}

std::shared_ptr<Entity> Scene::CreateEntity(const std::string& name, const UUID& uuid)
{
	std::shared_ptr<Entity> entity = std::make_shared<Entity>(shared_from_this(), name, uuid);
	m_Entities.emplace_back(entity);

	return entity;
}

std::shared_ptr<Entity> Scene::CloneEntity(std::shared_ptr<Entity> entity)
{
	std::function<std::shared_ptr<Entity>(std::shared_ptr<Entity>)> cloneEntity = [this, &cloneEntity](std::shared_ptr<Entity> entity)
	{
		std::shared_ptr<Entity> newEntity = CreateEntity(entity->GetName() + "Clone");

		for (auto [id, storage] : m_Registry.storage())
		{
			if (storage.contains(entity->GetHandle()))
			{
				storage.push(newEntity->GetHandle(), storage.value(entity->GetHandle()));
			}
		}

		// Transform and Camera requires to explicitly set the entity or set it as a constructor argument.
		if (newEntity->HasComponent<Transform>())
		{
			newEntity->GetComponent<Transform>().SetEntity(newEntity);
		}

		if (newEntity->HasComponent<Camera>())
		{
			newEntity->GetComponent<Camera>().SetEntity(newEntity);
		}

		for (std::shared_ptr<Entity> child : entity->GetChilds())
		{
			newEntity->AddChild(cloneEntity(child), false);
		}

		return newEntity;
	};

	std::shared_ptr<Entity> newEntity = cloneEntity(entity);

	if (entity->HasParent())
	{
		entity->GetParent()->AddChild(newEntity);
	}

	return newEntity;
}

void Scene::DeleteEntity(std::shared_ptr<Entity>& entity)
{
	while (!entity->GetChilds().empty())
	{
		std::shared_ptr<Entity> child = entity->GetChilds().back();
		DeleteEntity(child);
	}

	if (const std::shared_ptr<Entity> parent = entity->GetParent())
	{
		parent->RemoveChild(entity);
	}

	if (entity->GetHandle() != entt::tombstone)
	{
		m_Registry.destroy(entity->GetHandle());
	}

	if (const auto entityToErase = std::find(m_Entities.begin(), m_Entities.end(), entity);
		entityToErase != m_Entities.end())
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

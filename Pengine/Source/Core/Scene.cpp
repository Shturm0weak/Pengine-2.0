#include "Scene.h"

#include "Logger.h"
#include "Viewport.h"

#include "../Components/Transform.h"
#include "../Components/Renderer3D.h"
#include "../Components/Camera.h"

#include "../Core/MaterialManager.h"
#include "../Core/MeshManager.h"

using namespace Pengine;

Scene::Resources Scene::CollectResources(std::vector<std::shared_ptr<Entity>> entities)
{
	Resources resources{};

	for (auto entity : m_Entities)
	{
		if (entity->HasComponent<Renderer3D>())
		{
			const Renderer3D& r3d = entity->GetComponent<Renderer3D>();

			if (r3d.mesh && std::filesystem::exists(r3d.mesh->GetFilepath()))
			{
				resources.meshes.emplace(r3d.mesh);
			}

			if (r3d.material)
			{
				resources.materials.emplace(r3d.material);
			}

			if (r3d.material && r3d.material->GetBaseMaterial())
			{
				resources.baseMaterials.emplace(r3d.material->GetBaseMaterial());
			}
		}
	}

	return resources;
}

void Scene::DeleteResources(const Resources& resources)
{
	for (auto material : resources.materials)
	{
		MaterialManager::GetInstance().DeleteMaterial(material);
	}

	for (auto baseMaterial : resources.baseMaterials)
	{
		MaterialManager::GetInstance().DeleteBaseMaterial(baseMaterial);
	}

	for (auto mesh : resources.meshes)
	{
		MeshManager::GetInstance().DeleteMesh(mesh);
	}
}

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
	, m_GraphicsSettings("Default", "Default")
{

}

Scene::~Scene()
{
	Clear();
}

Scene::Scene(const Scene& scene)
	: Asset(scene.GetName(), scene.GetFilepath())
	, enable_shared_from_this(scene)
	, m_GraphicsSettings(scene.m_GraphicsSettings.GetName(), scene.m_GraphicsSettings.GetFilepath())
{
	Copy(scene);
}

void Scene::Clear()
{
	m_Name = none;
	m_Filepath = none;

	DeleteResources(CollectResources(m_Entities));

	m_Entities.clear();
	m_Registry.clear();
	m_SelectedEntities.clear();

	m_RenderTarget = nullptr;
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

		for (const std::weak_ptr<Entity> weakChild : entity->GetChilds())
		{
			if (const std::shared_ptr<Entity> child = weakChild.lock())
			{
				newEntity->AddChild(cloneEntity(child), false);
			}
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
		if (std::shared_ptr<Entity> child = entity->GetChilds().back().lock())
		{
			DeleteEntity(child);
		}
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

std::shared_ptr<Entity> Scene::FindEntityByName(const std::string& name)
{
	for (std::shared_ptr<Entity> entity : m_Entities)
	{
		if (entity->GetName() == name)
		{
			return entity;
		}
	}

	return nullptr;
}

#include "Scene.h"

#include "Logger.h"
#include "Viewport.h"
#include "FileFormatNames.h"
#include "ThreadPool.h"

#include "../Components/Transform.h"
#include "../Components/Renderer3D.h"
#include "../Components/Camera.h"
#include "../Components/DirectionalLight.h"
#include "../Components/PointLight.h"
#include "../Components/Canvas.h"

#include "../Core/MaterialManager.h"
#include "../Core/MeshManager.h"

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

void Scene::FlushDeletionQueue()
{
	while (!m_EntityDeletionQueue.empty())
	{
		const auto entity = m_EntityDeletionQueue.front();
		m_EntityDeletionQueue.pop();

		while (!entity->GetChilds().empty())
		{
			if (std::shared_ptr<Entity> child = entity->GetChilds().back().lock())
			{
				DeleteEntity(child);
			}
		}

		if (entity->GetHandle() != entt::tombstone)
		{
			for (const auto& [componentSystemName, componentSystem] : m_ComponentSystemsByName)
			{
				const auto removeCallbacks = std::move(componentSystem->GetRemoveCallbacks());
				for (const auto& [componentName, callback] : removeCallbacks)
				{
					if (callback)
					{
						callback(entity);
					}
				}
			}

			m_Registry.destroy(entity->GetHandle());
		}
	}
}

std::shared_ptr<Scene> Scene::Create(const std::string& name, const std::string& tag)
{
	std::shared_ptr<Scene> scene = std::make_shared<Scene>(name, none);
	scene->SetTag(tag);

	return scene;
}

Scene::Scene(const std::string& name, const std::filesystem::path& filepath)
	: Asset(name, filepath)
	, m_GraphicsSettings("Default", "Default")
{
	m_CurrentBVH = std::make_shared<SceneBVH>(this);
	m_BuildingBVH = std::make_shared<SceneBVH>(this);
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

	m_Entities.clear();
	m_Registry.clear();
	m_SelectedEntities.clear();

	m_RenderView = nullptr;
}

void Scene::ProcessComponentRemove(
	const std::string& componentName,
	std::shared_ptr<Entity> entity) const
{
	for (const auto& [componentSystemName, componentSystem] : m_ComponentSystemsByName)
	{
		const auto removeCallbacks = std::move(componentSystem->GetRemoveCallbacks());
		auto callback = removeCallbacks.find(componentName);
		if (callback != removeCallbacks.end())
		{
			if (callback->second)
			{
				callback->second(entity);
			}
		}
	}
}

std::shared_ptr<ComponentSystem> Scene::GetComponentSystem(const std::string& name)
{
	auto componentSystem = m_ComponentSystemsByName.find(name);
	if (componentSystem != m_ComponentSystemsByName.end())
	{
		return componentSystem->second;
	}

	return nullptr;
}

std::shared_ptr<Entity> Scene::CreateEmpty()
{
	const auto entity = CreateEntity("Empty");
	entity->AddComponent<Transform>(entity);
	return entity;
}

std::shared_ptr<Entity> Scene::CreateCamera()
{
	const auto entity = CreateEntity("Camera");
	entity->AddComponent<Transform>(entity);
	entity->AddComponent<Camera>(entity);
	return entity;
}

std::shared_ptr<Entity> Scene::CreateDirectionalLight()
{
	const auto entity = CreateEntity("DirectionalLight");
	entity->AddComponent<Transform>(entity);
	entity->AddComponent<DirectionalLight>();
	return entity;
}

std::shared_ptr<Entity> Scene::CreatePointLight()
{
	const auto entity = CreateEntity("PointLight");
	entity->AddComponent<Transform>(entity);
	entity->AddComponent<PointLight>();
	return entity;
}

std::shared_ptr<Entity> Scene::CreateCube()
{
	const auto entity = CreateEntity("Cube");
	entity->AddComponent<Transform>(entity);
	auto& r3d = entity->AddComponent<Renderer3D>();
	r3d.mesh = MeshManager::GetInstance().LoadMesh(std::filesystem::path("Meshes") / "Cube.mesh");
	r3d.material = MaterialManager::GetInstance().LoadMaterial(std::filesystem::path("Materials") / "MeshBase.mat");
	return entity;
}

std::shared_ptr<Entity> Scene::CreateSphere()
{

	const auto entity = CreateEntity("Sphere");
	entity->AddComponent<Transform>(entity);
	auto& r3d = entity->AddComponent<Renderer3D>();
	r3d.mesh = MeshManager::GetInstance().LoadMesh(std::filesystem::path("Meshes") / "Sphere.mesh");
	r3d.material = MaterialManager::GetInstance().LoadMaterial(std::filesystem::path("Materials") / "MeshBase.mat");
	return entity;
}

std::shared_ptr<Entity> Scene::CreateCanvas()
{
	const auto entity = CreateEntity("Canvas");
	entity->AddComponent<Transform>(entity);
	entity->AddComponent<Canvas>();
	auto& r3d = entity->AddComponent<Renderer3D>();
	r3d.mesh = MeshManager::GetInstance().LoadMesh(std::filesystem::path("Meshes") / "Plane.mesh");

	const std::shared_ptr<Material> defaultMaterial = MaterialManager::GetInstance().LoadMaterial(std::filesystem::path("Materials") / "UIBase.mat");
	const std::string name = std::to_string(UUID::Generate());
	std::filesystem::path filepath = defaultMaterial->GetFilepath().parent_path() / name;
	filepath.replace_extension(FileFormats::Mat());

	r3d.material = Material::Clone(name, filepath, defaultMaterial);
	return entity;
}

Scene& Scene::operator=(const Scene& scene)
{
	if(this != &scene)
	{
		Copy(scene);
	}

	return *this;
}

void Scene::Update(const float deltaTime)
{
	for (const auto& [name, system] : m_ComponentSystemsByName)
	{
		system->OnUpdate(deltaTime, shared_from_this());
	}

	{
		std::unique_lock<std::mutex> lock(m_LockBVH);
		m_BVHConditionalVariable.wait(lock, [this]
		{
			return !m_IsBuildingBVH;
		});

		std::swap(m_CurrentBVH, m_BuildingBVH);
	}

	FlushDeletionQueue();

	// TODO: Potential big problem, during rebuilding BVH entities can be added to the scene from other thread!
	ThreadPool::GetInstance().EnqueueAsync([this]()
	{
		std::lock_guard<std::mutex> lock(m_LockBVH);
		m_IsBuildingBVH = true;
		m_BuildingBVH->Update();
		m_IsBuildingBVH = false;
		m_BVHConditionalVariable.notify_all();
	});
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
		std::shared_ptr<Entity> newEntity = CreateEntity(entity->GetName());

		if (entity->IsPrefab())
		{
			newEntity->SetPrefabFilepathUUID(entity->GetPrefabFilepathUUID());
		}

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

	// At the moment of move constructor transform is created,
	// but at that moment entity of this transform is invalid,
	// so passing global transform to its child entities will not happen,
	// so here by copy we update the global transform of the whole hierarchy of entities.
	{
		Transform& transform = newEntity->GetComponent<Transform>();
		bool copyable = transform.IsCopyable();
		transform.SetCopyable(true);
		transform = entity->GetComponent<Transform>();
		transform.SetCopyable(copyable);
	}

	if (entity->HasParent())
	{
		entity->GetParent()->AddChild(newEntity, false);
	}

	return newEntity;
}

void Scene::DeleteEntity(std::shared_ptr<Entity>& entity)
{
	entity->SetDeleted(true);

	m_EntityDeletionQueue.emplace(entity);

	if (const std::shared_ptr<Entity> parent = entity->GetParent())
	{
		parent->RemoveChild(entity);
	}

	if (const auto entityToErase = std::find(m_Entities.begin(), m_Entities.end(), entity);
		entityToErase != m_Entities.end())
	{
		m_Entities.erase(entityToErase);
	}

	entity = nullptr;
}

std::shared_ptr<Entity> Scene::FindEntityByUUID(const UUID& uuid)
{
	for (std::shared_ptr<Entity> entity : m_Entities)
	{
		if (entity->GetUUID() == uuid)
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

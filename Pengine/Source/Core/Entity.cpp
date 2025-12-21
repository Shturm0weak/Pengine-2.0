#include "Entity.h"

#include "../Components/Transform.h"
#include "../Utils/Utils.h"

#include "Scene.h"

using namespace Pengine;

Entity::Entity(
	std::shared_ptr<Scene> scene,
	std::string name,
	UUID uuid)
	: m_Scene(std::move(scene))
	, m_Registry(&m_Scene.lock()->GetRegistry())
	, m_Name(std::move(name))
	, m_UUID(std::move(uuid))
{
	m_Handle = m_Registry->create();
}

Entity::Entity(const Entity& entity)
	: enable_shared_from_this(entity)
{
	Copy(entity);
}

Entity::Entity(Entity&& entity) noexcept
{
	Move(std::move(entity));
}

Entity::~Entity()
{

}

entt::registry& Entity::GetRegistry() const
{
	assert(m_Scene.lock());

	return *m_Registry;
}

std::shared_ptr<Scene> Entity::GetScene() const
{
	assert(m_Scene.lock());

	return m_Scene.lock();
}

Entity& Entity::operator=(const Entity& entity)
{
	if (this != &entity)
	{
		Copy(entity);
	}

	return *this;
}

Entity& Entity::operator=(Entity&& entity) noexcept
{
	Move(std::move(entity));
	return *this;
}

std::shared_ptr<Entity> Entity::GetTopEntity()
{
	if (const auto parent = GetParent())
	{
		return parent->GetTopEntity();
	}

	return shared_from_this();
}

std::shared_ptr<Entity> Entity::FindEntityInHierarchy(const std::string& name)
{
	if (GetName() == name)
	{
		return shared_from_this();
	}

	for (const std::weak_ptr<Entity>& weakChild : m_Childs)
	{
		std::shared_ptr<Entity> child = weakChild.lock();
		if (child->GetName() == name)
		{
			return child;
		}
		
		if (child = child->FindEntityInHierarchy(name))
		{
			return child;
		}
	}

	return nullptr;
}

void Entity::AddChild(const std::shared_ptr<Entity>& child, const bool saveTransform)
{
	if (HasComponent<Transform>() && child->HasComponent<Transform>())
	{
		const Transform& transform = GetComponent<Transform>();
		Transform& childTransform = child->GetComponent<Transform>();

		glm::vec3 position, rotation, scale;
		if (saveTransform)
		{
			Utils::DecomposeTransform(
				transform.GetInverseTransformMat4() * childTransform.GetTransform(),
				position,
				rotation,
				scale);
		}

		m_Childs.emplace_back(child);
		m_ChildEntities.emplace_back(child->GetHandle());
		child->SetParent(shared_from_this());

		if (saveTransform)
		{
			childTransform.Translate(position);
			childTransform.Rotate(rotation);
			childTransform.Scale(scale);
		}
	}
	else
	{
		m_Childs.emplace_back(child);
		m_ChildEntities.emplace_back(child->GetHandle());
		child->SetParent(shared_from_this());
	}
}

void Entity::RemoveChild(const std::shared_ptr<Entity>& child)
{
	if (HasComponent<Transform>() && child->HasComponent<Transform>())
	{
		Transform& childTransform = child->GetComponent<Transform>();

		glm::vec3 position, rotation, scale;
		Utils::DecomposeTransform(childTransform.GetTransform(), position, rotation, scale);

		if (auto childEntity = std::find(m_ChildEntities.begin(), m_ChildEntities.end(), child->GetHandle());
			childEntity != m_ChildEntities.end())
		{
			const size_t index = std::distance(m_ChildEntities.begin(), childEntity);
			m_Childs.erase(m_Childs.begin() + index);
			m_ChildEntities.erase(childEntity);
		}

		child->SetParent(nullptr);

		childTransform.Translate(position);
		childTransform.Rotate(rotation);
		childTransform.Scale(scale);
	}
	else
	{
		if (auto childEntity = std::find(m_ChildEntities.begin(), m_ChildEntities.end(), child->GetHandle());
			childEntity != m_ChildEntities.end())
		{
			const size_t index = std::distance(m_ChildEntities.begin(), childEntity);
			m_Childs.erase(m_Childs.begin() + index);
			m_ChildEntities.erase(childEntity);
		}
		child->SetParent(nullptr);
	}
}

bool Entity::HasAsChild(const std::shared_ptr<Entity>& child, const bool recursevely)
{
	if (auto childEntity = std::find(m_ChildEntities.begin(), m_ChildEntities.end(), child->GetHandle());
		childEntity != m_ChildEntities.end())
	{
		return true;
	}

	if (recursevely)
	{
		for (const std::weak_ptr<Entity> weakCurrentChild : m_Childs)
		{
			const std::shared_ptr<Entity> currentChild = weakCurrentChild.lock();
			if (currentChild && currentChild->HasAsChild(child, recursevely))
			{
				return true;
			}
		}
	}

	return false;
}

bool Entity::HasAsParent(const std::shared_ptr<Entity>& parent, const bool recursevely)
{
	if (!HasParent())
	{
		return false;
	}

	if (GetParent() == parent)
	{
		return true;
	}

	if (recursevely)
	{
		if (GetParent()->HasAsParent(parent))
		{
			return true;
		}
	}

	return false;
}

void Entity::SetUUID(const UUID& uuid)
{
	assert(m_Scene.lock());

	m_Scene.lock()->ReplaceEntityUUID(m_UUID, uuid);
	m_UUID = uuid;
}

bool Entity::IsEnabled() const
{
	bool enabled = true;

	std::shared_ptr<const Entity> entity = shared_from_this();
	while (entity)
	{
		enabled *= entity->m_IsEnabled && entity->IsValid();
		entity = entity->GetParent();
	}

	return enabled;
}

void Entity::Copy(const Entity& entity)
{
	m_Handle = entity.GetHandle();
	m_Scene = entity.GetScene();
	m_Parent = entity.GetParent();
	m_Childs = entity.GetChilds();
	m_Registry = &entity.GetRegistry();
	m_Name = entity.GetName();
	m_UUID = entity.GetUUID();
	m_IsEnabled = entity.IsEnabled();
	m_PrefabFilepathUUID = entity.GetPrefabFilepathUUID();
}

void Entity::Move(Entity&& entity) noexcept
{
	m_Handle = entity.m_Handle;
	m_Scene = entity.m_Scene;
	m_Parent = entity.m_Parent;
	m_Childs = std::move(entity.m_Childs);
	m_Registry = entity.m_Registry;
	m_Name = std::move(entity.m_Name);
	m_IsEnabled = entity.m_IsEnabled;
	m_PrefabFilepathUUID = std::move(entity.m_PrefabFilepathUUID);

	entity.m_Handle = entt::tombstone;
	entity.m_Scene.reset();
	entity.m_Parent.reset();
	entity.m_Childs.clear();
}

void Entity::NotifySceneAboutComponentRemove(const std::string& componentName)
{
	const auto scene = m_Scene.lock();
	if (scene)
	{
		scene->ProcessComponentRemove(componentName, shared_from_this());
	}
}

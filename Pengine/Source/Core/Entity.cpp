#include "Entity.h"

#include "../Components/Transform.h"
#include "../Utils/Utils.h"

#include "Scene.h"

using namespace Pengine;

Entity::Entity(std::shared_ptr<Scene> scene,
	const std::string& name,
	const UUID& uuid)
	: m_Scene(scene)
	, m_Registry(&m_Scene->GetRegistry())
	, m_Name(name)
	, m_UUID(uuid)
{
	m_Handle = m_Registry->create();
	//Logger::Log("Create Entity");
}

Entity::Entity(const Entity& entity)
{
	Copy(entity);
}

Entity::Entity(Entity&& entity) noexcept
{
	Move(std::move(entity));
}

Entity::~Entity()
{
	//Logger::Log("Destroy Entity");
}

entt::registry& Entity::GetRegistry() const
{
	assert(m_Scene);

	return *m_Registry;
}

std::shared_ptr<Scene> Entity::GetScene() const
{
	assert(m_Scene);

	return m_Scene;
}

void Entity::operator=(const Entity& entity)
{
	Copy(entity);
}

void Entity::operator=(Entity&& entity) noexcept
{
	Move(std::move(entity));
}

void Entity::AddChild(std::shared_ptr<Entity> child)
{
	if (HasComponent<Transform>() && child->HasComponent<Transform>())
	{
		Transform& transform = GetComponent<Transform>();
		Transform& childTransform = child->GetComponent<Transform>();

		const glm::vec3 position = Utils::GetPosition(glm::inverse(transform.GetTransform()) * childTransform.GetTransform());
		const glm::vec3 rotation = childTransform.GetRotation() - transform.GetRotation();
		const glm::vec3 scale = childTransform.GetScale() / transform.GetScale();

		m_Childs.emplace_back(child);
		child->SetParent(shared_from_this());

		childTransform.Translate(position);
		childTransform.Rotate(rotation);
		childTransform.Scale(scale);
	}
	else
	{
		m_Childs.emplace_back(child);
		child->SetParent(shared_from_this());
	}
}

void Entity::RemoveChild(std::shared_ptr<Entity> child)
{
	if (HasComponent<Transform>() && child->HasComponent<Transform>())
	{
		Transform& transform = GetComponent<Transform>();
		Transform& childTransform = child->GetComponent<Transform>();

		const glm::vec3 position = childTransform.GetPosition();
		const glm::vec3 rotation = childTransform.GetRotation();
		const glm::vec3 scale = childTransform.GetScale();

		auto childToErase = std::find(m_Childs.begin(), m_Childs.end(), child);
		if (childToErase != m_Childs.end())
		{
			m_Childs.erase(childToErase);
		}
		child->SetParent(nullptr);

		childTransform.Translate(position);
		childTransform.Rotate(rotation);
		childTransform.Scale(scale);
	}
	else
	{
		auto childToErase = std::find(m_Childs.begin(), m_Childs.end(), child);
		if (childToErase != m_Childs.end())
		{
			m_Childs.erase(childToErase);
		}
		child->SetParent(nullptr);
	}
}

bool Entity::HasAsChild(std::shared_ptr<Entity> child, bool recursevely)
{
	auto childToHave = std::find(m_Childs.begin(), m_Childs.end(), child);
	if (childToHave != m_Childs.end())
	{
		return true;
	}

	if (recursevely)
	{
		for (std::shared_ptr<Entity> child : m_Childs)
		{
			if (child->HasAsChild(child, recursevely))
			{
				return true;
			}
		}
	}

	return false;
}

bool Entity::HasAsParent(std::shared_ptr<Entity> parent, bool recursevely)
{
	if (!HasParent())
	{
		return false;
	}

	if (m_Parent == parent)
	{
		return true;
	}

	if (recursevely)
	{
		if (m_Parent->HasAsParent(parent))
		{
			return true;
		}
	}

	return false;
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

	//Logger::Log("Copy Entity");
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

	entity.m_Handle = entt::tombstone;
	entity.m_Scene = nullptr;
	entity.m_Parent = nullptr;
	entity.m_Childs.clear();

	//Logger::Log("Move Entity");
}

#include "Entity.h"

#include "../Components/Transform.h"
#include "../Utils/Utils.h"

#include "Scene.h"

using namespace Pengine;

Entity::Entity(std::shared_ptr<Scene> scene,
	std::string name,
	UUID uuid)
	: m_Scene(std::move(scene))
	, m_Registry(&m_Scene->GetRegistry())
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

void Entity::AddChild(const std::shared_ptr<Entity>& child)
{
	if (HasComponent<Transform>() && child->HasComponent<Transform>())
	{
		const Transform& transform = GetComponent<Transform>();
		Transform& childTransform = child->GetComponent<Transform>();

		glm::vec3 position, rotation, scale;
		Utils::DecomposeTransform(
			glm::inverse(transform.GetTransform()) * childTransform.GetTransform(),
			position,
			rotation,
			scale);

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

void Entity::RemoveChild(const std::shared_ptr<Entity>& child)
{
	if (HasComponent<Transform>() && child->HasComponent<Transform>())
	{
		Transform& childTransform = child->GetComponent<Transform>();

		glm::vec3 position, rotation, scale;
		Utils::DecomposeTransform(childTransform.GetTransform(), position, rotation, scale);

		if (const auto childToErase = std::find(m_Childs.begin(), m_Childs.end(), child);
			childToErase != m_Childs.end())
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
		if (const auto childToErase = std::find(m_Childs.begin(), m_Childs.end(), child);
			childToErase != m_Childs.end())
		{
			m_Childs.erase(childToErase);
		}
		child->SetParent(nullptr);
	}
}

bool Entity::HasAsChild(const std::shared_ptr<Entity>& child, const bool recursevely)
{
	if (const auto childToHave = std::find(m_Childs.begin(), m_Childs.end(), child);
		childToHave != m_Childs.end())
	{
		return true;
	}

	if (recursevely)
	{
		for (const std::shared_ptr<Entity>& currentChild : m_Childs)
		{
			if (currentChild->HasAsChild(child, recursevely))
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
}

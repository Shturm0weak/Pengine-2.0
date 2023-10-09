#include "GameObject.h"

#include "Logger.h"
#include "Scene.h"
#include "Time.h"

using namespace Pengine;

void GameObject::Copy(const GameObject& gameObject)
{
	m_Name = gameObject.m_Name;
	m_Transform = gameObject.m_Transform;
	m_ComponentManager = gameObject.m_ComponentManager;
	m_IsSerializable = gameObject.m_IsSerializable;
	m_IsEnabled = gameObject.m_IsEnabled;
	m_IsSelectable = gameObject.m_IsSelectable;

	while (m_Childs.size() > 0)
	{
		m_Scene->DeleteGameObject(m_Childs.back());
	}

	std::vector<GameObject*> childs = gameObject.GetChilds();
	for (GameObject* child : childs)
	{
		GameObject* newChild = m_Scene->CreateGameObject(child->m_Name, child->m_Transform);
		AddChild(newChild);
		*newChild = *child;
	}
}

GameObject::GameObject(entt::entity entity, class Scene* scene, const std::string& name,
	const Transform& transform, const UUID& uuid)
{
	assert(scene != nullptr);

	m_Entity = entity;
	m_Name = name;
	m_Transform = transform;
	m_Scene = scene;
	m_CreationTime = Time::GetTime();
	m_Transform.m_Owner = this;

	if (uuid.Get().empty() || m_Scene->FindGameObjectByUUID(uuid))
	{
		m_UUID.Generate();
	}
	else
	{
		m_UUID = uuid;
	}

#ifdef _DEBUG
	Logger::Log(m_Name + ": has been created!");
#endif
}

void GameObject::operator=(const GameObject& gameObject)
{
	Copy(gameObject);
}

GameObject::~GameObject()
{
	Utils::Erase<GameObject*>(m_Scene->m_GameObjects, this);
	m_Scene->m_GameObjectsByUUID.erase(GetUUID());

	while (m_Childs.size() > 0)
	{
		m_Scene->DeleteGameObject(m_Childs.back());
	}

	if (m_Owner)
	{
		m_Owner->RemoveChild(this);
	}

	m_ComponentManager.Clear();

#ifdef _DEBUG
	Logger::Log(m_Name + ": has been deleted!");
#endif
}

bool GameObject::IsEnabled()
{
	bool enabled = true;

	GameObject* owner = this;
	while (owner)
	{
		enabled *= owner->m_IsEnabled;
		owner = owner->GetOwner();
	}

	return enabled;
}

void GameObject::ForChilds(std::function<void(GameObject& child)> forChilds)
{
	for (GameObject* child : m_Childs)
	{
		forChilds(*child);
	}
}

void GameObject::AddChild(GameObject* child)
{
	if (!child)
	{
		return;
	}

	if (this == child || child == this->GetOwner()) return;
	if (child->GetOwner() != nullptr)
	{
		child->GetOwner()->RemoveChild(child);
	}
	if (std::find(m_Childs.begin(), m_Childs.end(), child) == m_Childs.end())
	{
		m_Childs.push_back(child);
	}

	m_Transform.AddChild(&child->m_Transform);
}

void GameObject::RemoveChild(GameObject* child)
{
	if (Utils::Erase<GameObject*>(m_Childs, child))
	{
		m_Transform.RemoveChild(&child->m_Transform);
	}
}

void GameObject::SetCopyableTransform(bool copyable)
{
	m_Transform.SetCopyable(copyable);
	ForChilds([copyable](GameObject& child)
		{
			child.m_Transform.SetCopyable(copyable);
		}
	);
}

GameObject* GameObject::GetChildByName(const std::string& name)
{
	for (GameObject* child : m_Childs)
	{
		if (child->GetName() == name)
		{
			return child;
		}
	}

	return nullptr;
}
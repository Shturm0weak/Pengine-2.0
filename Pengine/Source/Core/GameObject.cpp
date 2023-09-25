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
		m_Childs.back()->Delete();
	}

	std::vector<GameObject*> childs = gameObject.GetChilds();
	for (GameObject* child : childs)
	{
		GameObject* newChild = new GameObject(child->m_Name, child->m_Transform);// m_Scene->CreateGameObject(child->m_Name, child->m_Transform);
		AddChild(newChild);
		*newChild = *child;
	}
}

GameObject::GameObject(const GameObject& gameObject)
{
	Copy(gameObject);
}

GameObject::GameObject(const std::string& name, const Transform& transform)
{
	m_Name = name;
	m_Transform = transform;
}

void GameObject::operator=(const GameObject& gameObject)
{
	Copy(gameObject);
}

GameObject::~GameObject()
{
}

GameObject* GameObject::Create(Scene* scene, const std::string& name, const Transform& transform, const UUID& uuid)
{
	assert(scene != nullptr);

	GameObject* gameObject = new GameObject();// MemoryManager::GetInstance().Allocate<GameObject>();
	gameObject->m_Name = name;
	gameObject->m_Transform = transform;
	gameObject->m_Scene = scene;
	gameObject->m_CreationTime = Time::GetTime();
	gameObject->m_Transform.m_Owner = gameObject;

	if (uuid.Get().empty() || gameObject->m_Scene->FindGameObjectByUUID(uuid))
	{
		gameObject->m_UUID.Generate();
	}
	else
	{
		gameObject->m_UUID = uuid;
	}

#ifdef _DEBUG
	Logger::Log(gameObject->m_Name + ": has been created!");
#endif

	return gameObject;
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

void GameObject::Delete()
{
	while (m_Childs.size() > 0)
	{
		m_Childs.back()->Delete();
	}

	if (m_Owner)
	{
		m_Owner->RemoveChild(this);
	}

	m_ComponentManager.Clear();
	//MemoryManager::GetInstance().FreeMemory<GameObject>(static_cast<IAllocator*>(this));

#ifdef _DEBUG
	Logger::Log(m_Name + ": has been deleted!");
#endif

	delete this;
}

void GameObject::DeleteLater(float seconds)
{
	//float deleteTime = Time::GetTime();
	//Timer::SetCallback([deleteTime, this]
	//	{
	//		if (deleteTime >= this->GetCreationTime())
	//		{
	//			this->Delete();
	//		}
	//	}, seconds);
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
#include "ComponentManager.h"

#include "GameObject.h"

using namespace Pengine;

//template<>
//Transform* ComponentManager::GetComponent<Transform>()
//{
//	if (m_Owner)
//	{
//		return &m_Owner->m_Transform;
//	}
//}

void ComponentManager::Copy(const ComponentManager& componentManager)
{
	for (Component* copyComponent : componentManager.m_Components)
	{
		if (Component* component = GetComponent(copyComponent->GetType()))
		{
			component->Copy(*(copyComponent));
		}
		else
		{
			AddComponent(copyComponent->CreateCopy(m_Owner));
		}
	}
}

void ComponentManager::operator=(const ComponentManager& componentManager)
{
	Copy(componentManager);
}

Component* ComponentManager::GetComponent(const std::string& type)
{
	for (auto component : m_Components)
	{
		if (component->GetType() == type)
		{
			return component;
		}
	}
	return nullptr;
}

bool ComponentManager::AddComponent(Component* component)
{
	if (GetComponent(component->GetType()))
	{
		return false;
	}
	else
	{
		component->SetOwner(m_Owner);
		m_Components.push_back(component);
#ifdef _DEBUG
		Logger::Log(component->GetType() + ":has been added to GameObject!");
#endif
		return true;
	}
}
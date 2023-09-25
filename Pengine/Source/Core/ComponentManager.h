#pragma once

#include "Core.h"
#include "Logger.h"
#include "Component.h"

#include "../Utils/Utils.h"

#include <vector>

namespace Pengine
{

	class PENGINE_API ComponentManager
	{
	private:

		std::vector<Component*> m_Components;
		class GameObject* m_Owner = nullptr;

		void Copy(const ComponentManager& componentManager);
	public:

		const std::vector<Component*> GetComponents() const { return m_Components; }

		ComponentManager(class GameObject* owner) : m_Owner(owner) {}

		void operator=(const ComponentManager& componentManager);

		void Clear()
		{
			for (auto component : m_Components)
			{
				delete component;
			}
			m_Components.clear();
		}

		Component* GetComponent(const std::string& type);

		template<class T>
		T* GetComponent()
		{
			for (auto component : m_Components)
			{
				if (component->GetType() == GetTypeName<T>())
				{
					return static_cast<T*>(component);
				}
			}
			return nullptr;
		}

		/*template<>
		Transform* GetComponent<Transform>();*/

		template<typename T>
		T* GetComponentSubClass()
		{
			for (auto component : m_Components)
			{
				if (dynamic_cast<T*>(component) != nullptr)
				{
					return static_cast<T*>(component);
				}
			}

			return nullptr;
		}

		bool AddComponent(Component* component);

		template<class T>
		T* AddComponent()
		{
			Component* component = GetComponent<T>();
			if (component == nullptr)
			{
				component = T::Create(m_Owner);
				component->SetOwner(m_Owner);
				component->SetType(GetTypeName<T>());
				m_Components.push_back(component);
#ifdef _DEBUG
				Logger::Log(GetTypeName<T>() + ":has been added to GameObject!");
#endif
				return static_cast<T*>(component);
			}
			return static_cast<T*>(component);
		}

		template<class T>
		void RemoveComponent()
		{
			RemoveComponent(GetComponent<T>());
		}

		void RemoveComponent(Component* component)
		{
			if (component == nullptr) return;

			Utils::Erase<Component*>(m_Components, component);

			component->Delete();
		}
	};

}

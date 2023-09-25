#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API Component
	{
	public:
		Component() = default;
		virtual ~Component() = default;
		virtual void operator=(const Component& component) { Copy(component); }
		virtual void operator=(Component* component) { Copy(*component); }

		virtual Component* New(class GameObject* owner) = 0;

		virtual void Copy(const Component& component) { *this = component; }

		virtual void Delete() { delete this; }

		virtual Component* CreateCopy(class GameObject* newOwner);

		std::string GetType() const { return m_Type; }

		void SetType(const std::string& type) { m_Type = type; }

		class GameObject* GetOwner() const { return m_Owner; }

		void SetOwner(class GameObject* owner) { m_Owner = owner; }

	protected:
		std::string m_Type;

		class GameObject* m_Owner = nullptr;
	};

}

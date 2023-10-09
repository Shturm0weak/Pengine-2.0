#pragma once

#include "../Core/Core.h"
#include "../Core/Entity.h"

namespace Pengine
{
	class GameObjectC : public ComponentC
	{
	public:
		GameObjectC(Entity& entity, const std::string& name = "Unnamed",
			const UUID& uuid = UUID());
		GameObjectC(const GameObjectC& gameObject);
		GameObjectC(GameObjectC&& gameObject) noexcept;
		~GameObjectC();

		virtual ComponentC* CreateCopy(entt::entity handle) override;

		void Copy(const GameObjectC& gameObject);
		
		void Move(GameObjectC&& gameObject);

		const std::string& GetName() const { return m_Name; }

		const UUID& GetUUID() const { return m_UUID; }

		void SetName(const std::string name) { m_Name = name; }

		Entity& GetEntity() { return m_Entity; }
	private:

		Entity m_Entity;

		std::string m_Name;
		UUID m_UUID;
	};

}
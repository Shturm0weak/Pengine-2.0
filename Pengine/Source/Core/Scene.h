#pragma once

#include "Core.h"
#include "Asset.h"
#include "GameObject.h"
#include "Entity.h"

#include "../Components/GameObjectC.h"

namespace Pengine
{

	class PENGINE_API Scene : public Asset
	{
	public:
		Scene(const std::string& name, const std::string& filepath);
		Scene(const Scene& scene);
		void operator=(const Scene& scene);

		GameObjectC& CreateGameObjectC(const std::string& name = "Unnamed",
			const UUID& uuid = UUID());



















		GameObject* CreateGameObject(const std::string& name = "Unnamed",
			const Transform& transform = Transform(), const UUID& uuid = UUID());

		Entity CreateEntity();

		void DeleteEntity(Entity& entity);

		GameObject* FindGameObjectByName(const std::string& name);

		std::vector<GameObject*> FindGameObjects(const std::string& name);

		GameObject* FindGameObjectByUUID(const std::string& uuid);

		void DeleteGameObject(GameObject* gameObject);

		void DeleteGameObjectLater(GameObject* gameObject);

		const std::vector<GameObject*>& GetGameObjects() const { return m_GameObjects; }

		void Clear();

		void SetTag(const std::string& tag) { m_Tag = tag; }

		std::string GetTag() const { return m_Tag; }

		entt::registry& GetRegistry() { return m_Registry; }

		std::vector<class PointLight*> m_PointLights;
	private:
		std::unordered_map<std::string, GameObject*> m_GameObjectsByUUID;
		std::vector<GameObject*> m_GameObjects;

		std::string m_Tag = none;

		std::unordered_set<Entity> m_Entities;
		entt::registry m_Registry;

		void Copy(const Scene& scene);

		friend class GameObject;
	};

}
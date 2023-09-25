#pragma once

#include "Core.h"
#include "Asset.h"
#include "GameObject.h"

namespace Pengine
{

	class PENGINE_API Scene : public Asset
	{
	public:
		Scene(const std::string& name, const std::string& filepath);
		Scene(const Scene& scene);
		void operator=(const Scene& scene);

		GameObject* CreateGameObject(const std::string& name = "Unnamed",
			const Transform& transform = Transform(), const UUID& uuid = UUID());

		GameObject* FindGameObjectByName(const std::string& name);

		std::vector<GameObject*> FindGameObjects(const std::string& name);

		GameObject* FindGameObjectByUUID(const std::string& uuid);

		void DeleteGameObject(GameObject* gameObject);

		void DeleteGameObjectLater(GameObject* gameObject);

		const std::vector<GameObject*>& GetGameObjects() const { return m_GameObjects; }

		void Clear();

		void SetTag(const std::string& tag) { m_Tag = tag; }

		std::string GetTag() const { return m_Tag; }

		std::vector<class PointLight*> m_PointLights;
	private:
		std::unordered_map<std::string, GameObject*> m_GameObjectsByUUID;
		std::vector<GameObject*> m_GameObjects;

		std::string m_Tag = none;

		void Copy(const Scene& scene);
	};

}
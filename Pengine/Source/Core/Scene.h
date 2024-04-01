#pragma once

#include "Core.h"
#include "Asset.h"
#include "Entity.h"

namespace Pengine
{

	class PENGINE_API Scene : public Asset, public std::enable_shared_from_this<Scene>
	{
	public:
		Scene(const std::string& name, const std::filesystem::path& filepath);
		Scene(const Scene& scene);
		~Scene();
		Scene& operator=(const Scene& scene);

		std::shared_ptr<Entity> CreateEntity(const std::string& name = "Unnamed", const UUID& uuid = UUID());

		void DeleteEntity(std::shared_ptr<Entity>& entity);

		std::shared_ptr<Entity> FindEntityByUUID(const std::string& uuid);

		const std::vector<std::shared_ptr<Entity>>& GetEntities() const { return m_Entities; }

		void Clear();

		void SetTag(const std::string& tag) { m_Tag = tag; }

		void SetFilepath(const std::filesystem::path& filepath) { m_Filepath = filepath; }

		std::string GetTag() const { return m_Tag; }

		entt::registry& GetRegistry() { return m_Registry; }

		std::vector<class PointLight*> m_PointLights;
	private:
		std::unordered_map<std::string, std::shared_ptr<Entity>> m_EntitiesByUUID;
		std::vector<std::shared_ptr<Entity>> m_Entities;
		
		entt::registry m_Registry;

		std::string m_Tag = none;

		void Copy(const Scene& scene);
	};

}
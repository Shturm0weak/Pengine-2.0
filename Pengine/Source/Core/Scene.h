#pragma once

#include "Core.h"
#include "Asset.h"
#include "Entity.h"
#include "Visualizer.h"
#include "GraphicsSettings.h"

namespace Pengine
{

	class PENGINE_API Scene : public Asset, public std::enable_shared_from_this<Scene>
	{
	public:
		struct Settings
		{
			bool m_DrawBoundingBoxes = false;
		};

		Scene(const std::string& name, const std::filesystem::path& filepath);
		Scene(const Scene& scene);
		~Scene();
		Scene& operator=(const Scene& scene);

		std::shared_ptr<Entity> CreateEntity(const std::string& name = "Unnamed", const UUID& uuid = UUID());

		std::shared_ptr<Entity> CloneEntity(std::shared_ptr<Entity> entity);

		void DeleteEntity(std::shared_ptr<Entity>& entity);

		std::shared_ptr<Entity> FindEntityByUUID(const std::string& uuid);

		std::shared_ptr<Entity> FindEntityByName(const std::string& name);

		const std::vector<std::shared_ptr<Entity>>& GetEntities() const { return m_Entities; }

		void Clear();

		void SetTag(const std::string& tag) { m_Tag = tag; }

		void SetFilepath(const std::filesystem::path& filepath) { m_Filepath = filepath; }

		std::string GetTag() const { return m_Tag; }

		entt::registry& GetRegistry() { return m_Registry; }

		Visualizer& GetVisualizer() { return m_Visualizer; }

		Settings& GetSettings() { return m_Settings; }

		GraphicsSettings& GetGraphicsSettings() { return m_GraphicsSettings; }

		void SetGraphicsSettings(const GraphicsSettings& graphicsSettings) { m_GraphicsSettings = graphicsSettings; }

	private:
		std::unordered_map<std::string, std::shared_ptr<Entity>> m_EntitiesByUUID;
		std::vector<std::shared_ptr<Entity>> m_Entities;
		entt::registry m_Registry;
		Visualizer m_Visualizer;
		Settings m_Settings;
		GraphicsSettings m_GraphicsSettings;

		std::string m_Tag = none;

		void Copy(const Scene& scene);
	};

}
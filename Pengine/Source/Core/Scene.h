#pragma once

#include "Core.h"
#include "Asset.h"
#include "Entity.h"
#include "Visualizer.h"
#include "GraphicsSettings.h"

#include "../Graphics/RenderTarget.h"
#include "../ComponentSystems/ComponentSystem.h"

namespace Pengine
{

	class PENGINE_API Scene : public Asset, public std::enable_shared_from_this<Scene>
	{
	public:
		struct Settings
		{
			bool m_DrawBoundingBoxes = false;
		};

		static std::shared_ptr<Scene> Create(const std::string& name, const std::string& tag);

		Scene(const std::string& name, const std::filesystem::path& filepath);
		Scene(const Scene& scene);
		~Scene();
		Scene& operator=(const Scene& scene);

		void Update(const float deltaTime);

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

		std::set<std::shared_ptr<Entity>>& GetSelectedEntities() { return m_SelectedEntities; }

		std::shared_ptr<RenderTarget> GetRenderTarget() const { return m_RenderTarget; }

		void SetRenderTarget(std::shared_ptr<RenderTarget> renderTarget) { m_RenderTarget = renderTarget; }

		void SetComponentSystem(const std::string& name, std::shared_ptr<ComponentSystem> componentSystem) { m_ComponentSystemsByName[name] = componentSystem; }
	private:
		struct Resources
		{
			std::set<std::shared_ptr<class Mesh>> meshes;
			std::set<std::shared_ptr<class BaseMaterial>> baseMaterials;
			std::set<std::shared_ptr<class Material>> materials;
		};

		std::unordered_map<std::string, std::shared_ptr<ComponentSystem>> m_ComponentSystemsByName;

		std::vector<std::shared_ptr<Entity>> m_Entities;
		entt::registry m_Registry;
		Visualizer m_Visualizer;
		Settings m_Settings;
		GraphicsSettings m_GraphicsSettings;

		std::string m_Tag = none;

		std::set<std::shared_ptr<Entity>> m_SelectedEntities;

		std::shared_ptr<RenderTarget> m_RenderTarget;

		void Copy(const Scene& scene);
	};

}
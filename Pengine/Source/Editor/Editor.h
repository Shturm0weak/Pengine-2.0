#pragma once

#include "../Core/Core.h"
#include "../Core/Scene.h"
#include "../Components/Camera.h"
#include "../Components/Renderer3D.h"
#include "../Components/PointLight.h"

#include <filesystem>

namespace Pengine
{

	class Editor
	{
	public:
		Editor();

		void Update(std::shared_ptr<Scene> scene);

	private:
		class Indent
		{
		public:
			Indent();
			~Indent();
		};

		bool DrawVec2Control(const std::string& label, glm::vec2& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f);

		bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f);

		bool DrawVec4Control(const std::string& label, glm::vec4& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f);

		bool DrawIVec2Control(const std::string& label, glm::ivec2& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f);

		bool DrawIVec3Control(const std::string& label, glm::ivec3& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f);

		bool DrawIVec4Control(const std::string& label, glm::ivec4& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f);

		void Hierarchy(std::shared_ptr<Scene> scene);

		void DrawScene(std::shared_ptr<Scene> scene);

		void DrawNode(std::shared_ptr<Entity> entity, ImGuiTreeNodeFlags flags);

		void DrawChilds(std::shared_ptr<Entity> entity);

		void Properties(std::shared_ptr<Scene> scene);

		void CameraComponent(std::shared_ptr<Entity> entity);

		void TransformComponent(std::shared_ptr<Entity> entity);

		void Renderer3DComponent(std::shared_ptr<Entity> entity);

		void PointLightComponent(std::shared_ptr<Entity> entity);

		void GameObjectPopUpMenu(std::shared_ptr<Scene> scene);

		void ComponentsPopUpMenu(std::shared_ptr<Entity> entity);

		void AssetBrowser();

		void SetDarkThemeColors();

		void Manipulate();

		void MoveCamera(std::shared_ptr<Entity> camera);

		struct MaterialMenu
		{
			std::shared_ptr<Material> material = nullptr;
			bool opened = false;

			void Update(Editor& editor);
		} m_MaterialMenu;

		std::set<std::string> m_SelectedEntities;

		std::filesystem::path m_RootDirectory = std::filesystem::current_path();
		std::filesystem::path m_CurrentDirectory = m_RootDirectory;

		//char m_FilterBuffer[64];

		float m_ThumbnailScale = 0.8f;

		uint32_t m_GizmoOperation = 0;

		int m_TransformSystem = 0;

		bool m_DrawVecLabel = true;

		std::shared_ptr<Entity> m_MovingCamera;
	};

}
#pragma once

#include "../Core/Core.h"
#include "../Core/Scene.h"
#include "../Components/Transform.h"
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

		void DrawNode(GameObject* gameObject, ImGuiTreeNodeFlags flags);

		void DrawChilds(GameObject* gameObject);

		void Properties(std::shared_ptr<Scene> scene);

		void TransformComponent(Transform& transform);

		void Renderer3DComponent(Renderer3D* r3d);

		void PointLightComponent(PointLight* pointLight);

		void GameObjectPopUpMenu(std::shared_ptr<Scene> scene);

		void ComponentsPopUpMenu(GameObject* gameObject);

		void AssetBrowser();

		struct MaterialMenu
		{
			std::shared_ptr<Material> material = nullptr;
			bool opened = false;

			void Update(Editor& editor);
		} m_MaterialMenu;

		std::set<std::string> m_SelectedGameObjects;

		std::filesystem::path m_RootDirectory = std::filesystem::current_path();
		std::filesystem::path m_CurrentDirectory = m_RootDirectory;

		//char m_FilterBuffer[64];

		float m_ThumbnailScale = 0.8f;

		bool m_DrawVecLabel = true;
	};

}
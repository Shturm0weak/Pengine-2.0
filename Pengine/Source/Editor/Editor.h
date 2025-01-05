#pragma once

#include "../Core/Core.h"
#include "../Core/Scene.h"

#include <filesystem>

namespace Pengine
{

	class Editor
	{
	public:
		Editor();

		void Update(const std::shared_ptr<Scene>& scene);

	private:
		class Indent
		{
		public:
			Indent();
			~Indent();
		};

		bool DrawVec2Control(const std::string& label, glm::vec2& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f) const;

		bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f) const;

		bool DrawVec4Control(const std::string& label, glm::vec4& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f) const;

		bool DrawIVec2Control(const std::string& label, glm::ivec2& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f) const;

		bool DrawIVec3Control(const std::string& label, glm::ivec3& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f) const;

		bool DrawIVec4Control(const std::string& label, glm::ivec4& values, float resetValue = 0.0f,
			const glm::vec2& limits = glm::vec2(-25.0f, 25.0f), float speed = 0.1f, float columnWidth = 100.0f) const;

		bool ImageCheckBox(const void* id, ImTextureID textureOn, ImTextureID textureOff, bool& enabled);

		void Hierarchy(const std::shared_ptr<Scene>& scene);

		void SceneInfo(const std::shared_ptr<Scene>& scene);

		void DrawScene(const std::shared_ptr<Scene>& scene);

		void DrawNode(const std::shared_ptr<Entity>& entity, ImGuiTreeNodeFlags flags);

		void DrawChilds(const std::shared_ptr<Entity>& entity);

		void Properties(const std::shared_ptr<Scene>& scene);

		void GraphicsSettingsInfo(GraphicsSettings& graphicsSettings);

		void CameraComponent(const std::shared_ptr<Entity>& entity);

		void TransformComponent(const std::shared_ptr<Entity>& entity);

		void Renderer3DComponent(const std::shared_ptr<Entity>& entity);

		void PointLightComponent(const std::shared_ptr<Entity>& entity);

		void DirectionalLightComponent(const std::shared_ptr<Entity>& entity);

		void GameObjectPopUpMenu(const std::shared_ptr<Scene>& scene);

		void ComponentsPopUpMenu(const std::shared_ptr<Entity>& entity);

		void MainMenuBar();

		void AssetBrowser(const std::shared_ptr<Scene>& scene);

		void AssetBrowserHierarchy();

		void DrawAssetBrowserHierarchy(const std::filesystem::path& directory);

		void SetDarkThemeColors();

		void Manipulate(const std::shared_ptr<Scene>& scene);

		void MoveCamera(const std::shared_ptr<Entity>& camera);

		ImTextureID GetFileIcon(const std::filesystem::path& filepath, const std::string& format);

		struct MaterialMenu
		{
			std::shared_ptr<class Material> material = nullptr;
			bool opened = false;

			void Update(Editor& editor);
		} m_MaterialMenu;

		struct BaseMaterialMenu
		{
			std::shared_ptr<class BaseMaterial> baseMaterial = nullptr;
			bool opened = false;

			void Update(const Editor& editor);
		} m_BaseMaterialMenu;

		struct CloneMaterialMenu
		{
			char name[64];

			bool opened = false;

			std::shared_ptr<class Material> material = nullptr;

			void Update();
		} m_CloneMaterialMenu;

		struct CreateFileMenu
		{
			char name[64];

			bool opened = false;

			std::filesystem::path filepath;
			std::string format;

			void Update();

			static void MakeFile(const std::filesystem::path& filepath);
		} m_CreateFileMenu;

		struct DeleteFileMenu
		{
			bool opened = false;

			std::filesystem::path filepath;

			void Update();
		} m_DeleteFileMenu;

		struct CreateViewportMenu
		{
			bool opened = false;

			char name[64];
			glm::ivec2 size = { 1024, 1024 };

			void Update(const Editor& editor);
		} m_CreateViewportMenu;

		struct LoadIntermediateMenu
		{
			bool opened = false;

			std::string workName = none;
			float workStatus = 0.0f;

			void Update();
		} m_LoadIntermediateMenu;

		struct TextureMetaPropertiesMenu
		{
			bool opened = false;

			Texture::Meta meta{};

			void Update();
		} m_TextureMetaPropertiesMenu;

		std::filesystem::path m_RootDirectory = std::filesystem::current_path();
		std::filesystem::path m_CurrentDirectory = m_RootDirectory;

		char m_AssetBrowserFilterBuffer[64];

		float m_ThumbnailScale = 0.8f;

		int m_TransformSystem = 0;

		bool m_DrawVecLabel = true;

		bool m_FullScreen = false;

		std::shared_ptr<Entity> m_MovingCamera;
	};

}
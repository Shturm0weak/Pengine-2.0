#pragma once

#include "ThumbnailAtlas.h"

#include "../Core/Core.h"
#include "../Core/GraphicsSettings.h"
#include "../Graphics/Texture.h"

#include "EntityAnimatorEditor.h"

#include <deque>

namespace Pengine
{

	class Scene;
	class Entity;
	class Window;

	class Editor
	{
	public:
		Editor();

		void Update(const std::shared_ptr<Scene>& scene, Window& window);

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

		void Hierarchy(const std::shared_ptr<Scene>& scene, Window& window);

		void SceneInfo(const std::shared_ptr<Scene>& scene);

		void DrawScene(const std::shared_ptr<Scene>& scene, Window& window);

		void DrawNode(const std::shared_ptr<Entity>& entity, ImGuiTreeNodeFlags flags, Window& window);

		void DrawChilds(const std::shared_ptr<Entity>& entity, Window& window);

		void Properties(const std::shared_ptr<Scene>& scene, Window& window);

		void GraphicsSettingsInfo(GraphicsSettings& graphicsSettings);

		void CameraComponent(const std::shared_ptr<Entity>& entity, Window& window);

		void TransformComponent(const std::shared_ptr<Entity>& entity);

		void Renderer3DComponent(const std::shared_ptr<Entity>& entity);

		void PointLightComponent(const std::shared_ptr<Entity>& entity);

		void DirectionalLightComponent(const std::shared_ptr<Entity>& entity);

		void SkeletalAnimatorComponent(const std::shared_ptr<Entity>& entity);

		void EntityAnimatorComponent(const std::shared_ptr<Entity>& entity);

		void CanvasComponent(const std::shared_ptr<Entity>& entity);

		void GameObjectPopUpMenu(const std::shared_ptr<Scene>& scene);

		void ComponentsPopUpMenu(const std::shared_ptr<Entity>& entity);

		void MainMenuBar(const std::shared_ptr<Scene>& scene);

		void AssetBrowser(const std::shared_ptr<Scene>& scene);

		void AssetBrowserHierarchy();

		void DrawAssetBrowserHierarchy(const std::filesystem::path& directory);

		void SetDarkThemeColors();

		void Manipulate(const std::shared_ptr<Scene>& scene, Window& window);

		void MoveCamera(const std::shared_ptr<Entity>& camera, Window& window);

		void SaveScene(std::shared_ptr<Scene> scene);

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

			void Update(const Editor& editor, Window& window);
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

		struct ImportMenu
		{
			bool opened = false;

			std::filesystem::path filepath;

			struct Options
			{
				bool importMeshes = true;
				bool importSkeletons = true;
				bool importMaterials = true;
				bool importAnimations = true;
				bool importPrefabs = true;
			} options;

			void Update(Editor& editor);
		} m_ImportMenu;

		std::filesystem::path m_RootDirectory = std::filesystem::current_path();
		std::filesystem::path m_CurrentDirectory = m_RootDirectory;

		char m_AssetBrowserFilterBuffer[64];

		float m_ThumbnailScale = 1.0f;

		int m_TransformSystem = 0;

		bool m_DrawVecLabel = true;

		bool m_FullScreen = false;

		std::shared_ptr<Entity> m_MovingCamera;

		class Thumbnails
		{
		public:
			std::shared_ptr<Scene> m_ThumbnailScene;
			std::shared_ptr<Window> m_ThumbnailWindow;
			std::shared_ptr<class Renderer> m_ThumbnailRenderer;
			UUID m_CameraUUID;

			std::atomic<bool> m_IsThumbnailLoading = false;

			ThumbnailAtlas m_ThumbnailAtlas;

			enum class Type
			{
				MESH,
				MAT,
				SCENE,
				PREFAB,
				TEXTURE
			};

			struct ThumbnailLoadInfo
			{
				std::filesystem::path thumbnailFilepath;
				std::filesystem::path resourceFilepath;
				std::shared_ptr<Scene> scene;
				Type type;
			};
			std::deque<ThumbnailLoadInfo> m_ThumbnailQueue;
			std::unordered_map<std::filesystem::path, bool> m_GeneratingThumbnails;
			std::unordered_map<std::filesystem::path, ThumbnailAtlas::TileInfo> m_CacheThumbnails;
			std::unordered_map<std::filesystem::path, ThumbnailAtlas::TileInfo>::iterator m_ThumbnailToCheck = m_CacheThumbnails.end();

			void Initialize();

			void UpdateThumbnails();

			void UpdateMatMeshThumbnail(const ThumbnailLoadInfo& thumbnailLoadInfo);

			void UpdateScenePrefabThumbnail(const ThumbnailLoadInfo& thumbnailLoadInfo);

			void UpdateTextureThumbnail(const ThumbnailLoadInfo& thumbnailLoadInfo);

			ThumbnailAtlas::TileInfo GetOrGenerateThumbnail(
				const std::filesystem::path& filepath,
				std::shared_ptr<Scene> scene,
				Type type);

			ThumbnailAtlas::TileInfo TryGetThumbnail(const std::filesystem::path& filepath);
		} m_Thumbnails;

		ThumbnailAtlas::TileInfo GetFileIcon(const std::filesystem::path& filepath, const std::string& format);

		EntityAnimatorEditor m_EntityAnimatorEditor;
		bool m_EntityAnimatorEditorOpened = false;

		struct CopyTransform
		{
			glm::vec3 position{ 0.0f, 0.0f, 0.0f };
			glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
			glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
		} m_CopyTransform;

		friend class EntityAnimatorEditor;
	};

}

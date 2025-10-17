#pragma once

#include "../Core/Core.h"
#include "../Core/Entity.h"
#include "../Graphics/RenderPass.h"

namespace Pengine
{

	class Renderer;

	class PENGINE_API Camera
	{
	public:
		enum class Type
		{
			ORTHOGRAPHIC,
			PERSPECTIVE
		};

		explicit Camera(std::shared_ptr<Entity> entity);
		Camera(const Camera& camera);
		Camera(Camera&& camera) noexcept;
		Camera& operator=(const Camera& camera);
		Camera& operator=(Camera&& camera) noexcept;

		void SetEntity(std::shared_ptr<Entity> entity);

		[[nodiscard]] glm::mat4 GetViewMat4() const { return m_ViewMat4; }

		[[nodiscard]] Type GetType() const { return m_Type; }

		void SetType(Type type);

		void CreateRenderView(const std::string& name, const glm::ivec2& size);

		void ResizeRenderView(const std::string& name, const glm::ivec2& size);

		void DeleteRenderView(const std::string& name);

		[[nodiscard]] float GetFov() const { return m_Fov; }

		void SetFov(float fov);

		[[nodiscard]] float GetZNear() const { return m_Znear; }

		void SetZNear(float zNear);

		[[nodiscard]] float GetZFar() const { return m_Zfar; }

		void SetZFar(float zFar);

		[[nodiscard]] std::shared_ptr<RenderView> GetRendererTarget(const std::string& name) const;

		[[nodiscard]] std::shared_ptr<Entity> GetEntity() const { return m_Entity; }

		[[nodiscard]] const std::string& GetPassName() const { return m_PassName; }

		[[nodiscard]] int GetRenderTargetIndex() const { return m_RenderTargetIndex; }

		void SetPassName(const std::string& passName) { m_PassName = passName; }

		void SetRenderTargetIndex(int renderTargetIndex) { m_RenderTargetIndex = renderTargetIndex; }

		std::shared_ptr<class Texture> TakeScreenshot(const std::filesystem::path& filepath, const std::string& viewportName, bool* isLoaded);

		uint8_t GetObjectVisibilityMask() const { return m_ObjectVisibilityMask; }

		void SetObjectVisibilityMask(uint8_t objectVisibilityMask) { m_ObjectVisibilityMask = objectVisibilityMask; }

		uint8_t GetShadowVisibilityMask() const { return m_ShadowVisibilityMask; }

		void SetShadowVisibilityMask(uint8_t shadowVisibilityMask) { m_ShadowVisibilityMask = shadowVisibilityMask; }

	private:
		glm::mat4 m_ViewMat4{};

		std::unordered_map<std::string, std::shared_ptr<RenderView>> m_RenderViewsByName;

		std::shared_ptr<Entity> m_Entity;

		float m_Znear = 0.1f;
		float m_Zfar = 1000.0f;
		float m_Fov = glm::radians(90.0f);

		Type m_Type = Type::ORTHOGRAPHIC;

		uint8_t m_ObjectVisibilityMask = -1;
		uint8_t m_ShadowVisibilityMask = -1;

		std::string m_PassName = AntiAliasingAndCompose;
		int m_RenderTargetIndex = 0;

		void Copy(const Camera& camera);

		void Move(Camera&& camera) noexcept;

		void UpdateViewMat4();
	};

}

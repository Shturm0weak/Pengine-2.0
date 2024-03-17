#pragma once

#include "../Core/Core.h"
#include "../Core/Entity.h"

namespace Pengine
{

	class PENGINE_API Transform
	{
	public:
		enum class System
		{
			LOCAL,
			GLOBAL
		};

	private:

		glm::mat4 m_TransformMat4{};
		glm::mat4 m_PositionMat4{};
		glm::mat4 m_RotationMat4{};
		glm::mat4 m_ScaleMat4{};
		glm::vec3 m_PreviousPosition{};
		glm::vec3 m_PositionDelta{};
		glm::vec3 m_PreviousScale{};
		glm::vec3 m_ScaleDelta{};
		glm::vec3 m_Rotation{};
		glm::vec3 m_PreviousRotation{};
		glm::vec3 m_RotationDelta{};
		glm::vec3 m_Back{};
		glm::vec3 m_Up{};

		std::unordered_map<std::string, std::function<void()>> m_OnRotationCallbacks;
		std::unordered_map<std::string, std::function<void()>> m_OnTranslationCallbacks;
		std::unordered_map<std::string, std::function<void()>> m_OnScaleCallbacks;

		std::shared_ptr<Entity> m_Entity;

		bool m_FollowOwner = true;
		bool m_Copyable = true;
		bool m_IsDirty = true;

		void Copy(const Transform& transform);
		void Move(Transform&& transform) noexcept;
		void UpdateVectors();
		void UpdateTransforms();

	public:
		~Transform() = default;
		Transform(const Transform& transform);
		Transform(Transform&& transform) noexcept;
		explicit Transform(
			std::shared_ptr<Entity> entity,
			const glm::vec3& position = glm::vec3(0.0f),
			const glm::vec3& scale = glm::vec3(1.0f),
			const glm::vec3& rotation = glm::vec3(0.0f)
		);
		Transform& operator=(const Transform& transform);
		Transform& operator=(Transform&& transform) noexcept;

		void CopyGlobal(const Transform& transform);

		[[nodiscard]] glm::mat4 GetPositionMat4(System system = System::GLOBAL) const;
		
		[[nodiscard]] glm::mat4 GetRotationMat4(System system = System::GLOBAL) const;
		
		[[nodiscard]] glm::mat4 GetScaleMat4(System system = System::GLOBAL) const;
		
		[[nodiscard]] glm::vec3 GetPreviousPosition(System system = System::GLOBAL) const;
		
		[[nodiscard]] glm::vec3 GetPositionDelta(System system = System::GLOBAL) const;
		
		[[nodiscard]] glm::vec3 GetPosition(System system = System::GLOBAL) const;
		
		[[nodiscard]] glm::vec3 GetRotation(System system = System::GLOBAL) const;
		
		[[nodiscard]] glm::vec3 GetScale(System system = System::GLOBAL) const;
		
		[[nodiscard]] glm::vec3 GetBack() const { return m_Back; }
		
		[[nodiscard]] glm::vec3 GetUp() const { return m_Up; }
		
		[[nodiscard]] glm::vec3 GetForward() const { return glm::normalize(glm::vec3(-m_Back.x, m_Back.y, -m_Back.z)); }
		
		[[nodiscard]] glm::vec3 GetRight() const { return glm::normalize(glm::cross(GetForward(), GetUp())); }
		
		[[nodiscard]] glm::mat4 GetTransform(System system = System::GLOBAL) const;

		[[nodiscard]] glm::mat3 GetInverseTransform(System system = System::GLOBAL) const;
		
		[[nodiscard]] bool GetFollorOwner() const { return m_FollowOwner; }
		
		void SetFollowOwner(const bool followOwner) { m_FollowOwner = followOwner; }

		[[nodiscard]] bool IsCopyable() const { return m_Copyable; }

		void SetCopyable(const bool copyable) { m_Copyable = copyable; }
		
		void SetOnRotationCallback(const std::string& label, const std::function<void()>& callback) { m_OnRotationCallbacks.emplace(label, callback); }
		
		void SetOnTranslationCallback(const std::string& label, const std::function<void()>& callback) { m_OnTranslationCallbacks.emplace(label, callback); }

		void SetOnScaleCallback(const std::string& label, const std::function<void()>& callback) { m_OnScaleCallbacks.emplace(label, callback); }

		void RemoveOnRotationCallback(const std::string& label);

		void RemoveOnTranslationCallback(const std::string& label);

		void RemoveOnScaleCallback(const std::string& label);

		void ClearOnRotationCallbacks() { m_OnRotationCallbacks.clear(); }

		void ClearOnTranslationCallbacks() { m_OnTranslationCallbacks.clear(); }
		
		void ClearOnScaleCallbacks() { m_OnScaleCallbacks.clear(); }

		void Translate(const glm::vec3& position);
		
		void Rotate(const glm::vec3& rotation);
		
		void Scale(const glm::vec3& scale);
	};

}

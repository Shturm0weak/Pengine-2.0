#pragma once

#include "Core.h"
#include "ComponentManager.h"
#include "UUID.h"

#include "../Components/Transform.h"
#include "../Graphics/BaseMaterial.h"
#include "../Graphics/Mesh.h"

namespace Pengine
{

	class PENGINE_API GameObject
	{
	public:
		GameObject(entt::entity entity, class Scene* scene, const std::string& name = "Unnamed",
			const Transform& transform = Transform(), const UUID& uuid = UUID());
		~GameObject();
		void operator=(const GameObject& gameObject);

		Transform m_Transform;
		ComponentManager m_ComponentManager = ComponentManager(this);

		void Copy(const GameObject& gameObject);

		bool IsSerializable() const { return m_IsSerializable; }

		void SetSerializable(bool isSerializable) { m_IsSerializable = isSerializable; }

		bool IsSelectable() const { return m_IsSelectable; }

		void SetSelectable(bool isSelectable) { m_IsSelectable = isSelectable; }

		bool IsEnabled();

		void SetEnabled(bool isEnabled) { m_IsEnabled = isEnabled; }

		bool IsEditorVisible() const { return m_IsEditorVisible; }

		void SetEditorVisible(bool isEditorVisible) { m_IsEditorVisible = isEditorVisible; }

		float GetCreationTime() const { return m_CreationTime; }

		std::string GetName() const { return m_Name; }

		std::string GetUUID() const { return m_UUID.Get(); }

		void SetName(const std::string& name) { m_Name = name; }

		const std::vector<GameObject*>& GetChilds() const { return m_Childs; }

		void ForChilds(std::function<void(GameObject& child)> forChilds);

		void AddChild(GameObject* child);

		void RemoveChild(GameObject* child);

		void SetCopyableTransform(bool copyable);

		GameObject* GetChildByName(const std::string& name);

		bool HasOwner() const { return m_Owner != nullptr; }

		GameObject* GetOwner() const { return m_Owner; }

		void SetOwner(GameObject* owner) { m_Owner = owner; }

		class Scene* GetScene() const { return m_Scene; }

		entt::entity GetEntity() const { return m_Entity; }
	private:

		std::vector<GameObject*> m_Childs;
		std::string m_Name = "Unnamed";

		GameObject* m_Owner = nullptr;
		class Scene* m_Scene = nullptr;

		UUID m_UUID = UUID();

		entt::entity m_Entity;

		float m_CreationTime = 0.0f;

		bool m_IsSerializable = true;
		bool m_IsEnabled = true;
		bool m_IsSelectable = true;
		bool m_IsEditorVisible = true;
	};

}
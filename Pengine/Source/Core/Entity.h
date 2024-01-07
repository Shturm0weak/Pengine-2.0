#pragma once

#include "Core.h"

namespace Pengine
{

	class Scene;

	class PENGINE_API Entity : public std::enable_shared_from_this<Entity>
	{
	public:
		Entity(
			std::shared_ptr<Scene> scene,
			const std::string& name = "Unnamed",
			const UUID& uuid = UUID());
		Entity(const Entity& entity);
		Entity(Entity&& entity) noexcept;
		~Entity();

		entt::registry& GetRegistry() const;

		std::shared_ptr<Scene> GetScene() const;

		entt::entity GetHandle() const { return m_Handle; }

		std::size_t operator()() const { return (size_t)m_Handle; }

		bool operator==(const Entity& entity) const { return m_Handle == entity.m_Handle; }

		void operator=(const Entity& entity);

		void operator=(Entity&& entity) noexcept;

		template<typename T, typename ...Args>
		T& AddComponent(Args&&... args)
		{
			return m_Registry->emplace<T>(m_Handle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent() const
		{
			return m_Registry->get<T>(m_Handle);
		}

		template<typename T>
		bool HasComponent() const
		{
			return m_Registry->any_of<T>(m_Handle);
		}

		template<typename T>
		void RemoveComponent()
		{
			m_Registry->remove<T>(m_Handle);
		}

		bool HasParent() const { return m_Parent != nullptr && m_Handle != entt::tombstone; }

		void SetParent(std::shared_ptr<Entity> parent) { m_Parent = parent; }

		std::shared_ptr<Entity> GetParent() const { return m_Parent; }

		void AddChild(std::shared_ptr<Entity> child);

		void RemoveChild(std::shared_ptr<Entity> child);

		const std::vector<std::shared_ptr<Entity>>& GetChilds() const { return m_Childs; }

		const std::string& GetName() const { return m_Name; }

		void SetName(const std::string name) { m_Name = name; }

		const UUID& GetUUID() const { return m_UUID; }

		bool IsEnabled() const { return m_IsEnabled; }

		void SetEnabled(bool isEnabled) { m_IsEnabled = isEnabled; }

	private:
		void Copy(const Entity& entity);

		void Move(Entity&& entity) noexcept;

		entt::entity m_Handle{entt::tombstone};
		std::shared_ptr<Entity> m_Parent;
		std::vector<std::shared_ptr<Entity>> m_Childs;
		std::shared_ptr<Scene> m_Scene;
		entt::registry* m_Registry;
		std::string m_Name;
		UUID m_UUID;

		bool m_IsEnabled = true;

		friend class Scene;
	};

}

template<>
struct std::hash<Pengine::Entity>
{
	std::size_t operator()(const Pengine::Entity& entity) const noexcept
	{
		return (std::size_t)entity.GetHandle();
	}
};
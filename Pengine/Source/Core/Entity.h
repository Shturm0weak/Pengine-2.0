#pragma once

#include "Core.h"

namespace Pengine
{

	class Scene;

	class PENGINE_API Entity
	{
	public:
		Entity() = default;
		Entity(Scene* scene);
		Entity(const Entity& entity);
		Entity(Entity&& entity) noexcept;
		~Entity();

		Scene& GetScene();

		bool IsValid() const { return m_Handle != entt::tombstone; }

		entt::entity GetHandle() const { return m_Handle; }

		std::size_t operator()() const { return (size_t)m_Handle; }

		bool operator==(const Entity& entity) const { return m_Handle == entity.m_Handle; }

		void operator=(const Entity& entity);

		template<typename T, typename ...Args>
		T& AddComponent(Args&&... args)
		{
			return m_Scene->GetRegistry().emplace<T>(m_Handle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			return m_Scene->GetRegistry().get<T>(m_Handle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->GetRegistry().any_of<T>(m_Handle);
		}

		template<typename T>
		void RemoveComponent()
		{
			m_Scene->GetRegistry().remove<T>(m_Handle);
		}

		Entity Clone();

		entt::entity m_Handle{entt::tombstone};

		Scene* m_Scene = nullptr;

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
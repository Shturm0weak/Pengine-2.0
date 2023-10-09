#include "Entity.h"

#include "Scene.h"

using namespace Pengine;

Entity::Entity(Scene* scene)
	: m_Scene(scene)
{
	m_Handle = m_Scene->GetRegistry().create();
	//Logger::Log("Create Entity");
}

Entity::Entity(const Entity& entity)
	: m_Scene(entity.m_Scene)
	, m_Handle(entity.m_Handle)
{
	//Logger::Log("Copy Entity");
}

Entity::Entity(Entity&& entity) noexcept
	: m_Scene(entity.m_Scene)
	, m_Handle(entity.m_Handle)
{
	entity.m_Handle = entt::tombstone;
	//Logger::Log("Move Entity");
}

Entity::~Entity()
{
	//Logger::Log("Destroy Entity");
}

Scene& Entity::GetScene()
{
	assert(m_Scene);

	return *m_Scene;
}

void Entity::operator=(const Entity& entity)
{
	m_Scene = entity.m_Scene;
	m_Handle = entity.m_Handle;
}

Entity Entity::Clone()
{
	Entity entity = m_Scene->CreateEntity();
	for (auto& set : m_Scene->GetRegistry().storage())
	{
		ComponentC* component = static_cast<ComponentC*>(set.second.value(m_Handle));

		// TODO: Very bad decision to check type cast conversion like this.
		if (component->validation == "CreateCopy")
		{
			component->CreateCopy(entity.m_Handle);
		}
	}

	return entity;
}

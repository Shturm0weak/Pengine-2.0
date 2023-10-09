#include "GameObjectC.h"

#include "../Core/Scene.h"

using namespace Pengine;

GameObjectC::GameObjectC(Entity& entity, const std::string& name, const UUID& uuid)
	: m_Entity(entity)
	, m_Name(name)
{
	if (uuid.Get().empty())
	{
		m_UUID.Generate();
	}

	Logger::Log("Create GameObject");
}

GameObjectC::GameObjectC(const GameObjectC& gameObject)
	: m_Entity(gameObject.m_Entity)
{
	Copy(gameObject);

	Logger::Log("Copy GameObject");
}

GameObjectC::GameObjectC(GameObjectC&& gameObject) noexcept
	: m_Entity(gameObject.m_Entity)
{
	m_Name = std::move(gameObject.m_Name);
	m_UUID = std::move(gameObject.m_UUID);

	gameObject.m_Name.clear();

	Logger::Log("GameObject Move");
}

GameObjectC::~GameObjectC()
{
	Logger::Log("GameObject Destroy");
}

ComponentC* GameObjectC::CreateCopy(entt::entity handle)
{
	Entity entity;
	entity.m_Handle = handle;
	entity.m_Scene = &m_Entity.GetScene();
	return static_cast<ComponentC*>(&entity.AddComponent<GameObjectC>(entity, "Clone"));
}

void GameObjectC::Copy(const GameObjectC& gameObject)
{
	m_Name = gameObject.m_Name;
	m_UUID = gameObject.m_UUID;
}

void GameObjectC::Move(GameObjectC&& gameObject)
{
	m_Name = std::move(gameObject.m_Name);
	m_UUID = std::move(gameObject.m_UUID);

	gameObject.m_Name.clear();
}
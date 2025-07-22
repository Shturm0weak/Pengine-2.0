#pragma once

#include "Core/Application.h"

namespace Pengine
{
	class Scene;
	class Entity;
}

namespace JPH
{
	class Character;
}

class CharacterControllerExample : public Pengine::Application
{
public:

	virtual void OnPreStart() override;

	virtual void OnStart() override;

	virtual void OnUpdate() override;

	virtual void OnClose() override;

private:
	std::shared_ptr<class Pengine::Scene> m_Scene;

	struct Character
	{
		std::shared_ptr<class JPH::Character> joltCharacter;
		std::shared_ptr<class Pengine::Entity> entity;
		float height = 2.0f;
		float radius = 0.5f;
	} m_Character;
};

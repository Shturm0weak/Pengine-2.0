#pragma once

#include "Core/Core.h"
#include "Core/Input.h"

struct FirstPersonCharacter;

enum class State
{
	IDLE,
	WALK,
	RUN,
	SHOT,
	INSPECT,
	RELOAD
};

class CharacterState
{
public:
	virtual ~CharacterState() = default;
	virtual void Enter(entt::registry& registry, entt::entity entity) = 0;
	virtual void Update(entt::registry& registry, entt::entity entity, float deltaTime) = 0;
	virtual void Exit(entt::registry& registry, entt::entity entity) = 0;
	virtual void HandleInput(entt::registry& registry, entt::entity entity, Pengine::Input& input) = 0;
	virtual State GetState() const = 0;
};
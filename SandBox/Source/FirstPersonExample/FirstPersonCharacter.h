#pragma once

#include "Core/ReflectionSystem.h"
#include "Graphics/SkeletalAnimation.h"

namespace JPH
{
	class Character;
}

struct Enemy
{
	PROPERTY(float, health, 100.0f);
};
REGISTER_CLASS(Enemy);

struct FirstPersonCharacter
{
	enum class Action
	{
		IDLE,
		WALK,
		RUN,
		ONESHOT,
		INSPECT,
		RELOAD
	};

	std::map<Action, std::shared_ptr<Pengine::SkeletalAnimation>> animations;
	std::map<Action, float> transitionTimes;
	
	std::shared_ptr<class JPH::Character> joltCharacter;
	float height = 2.0f;
	float radius = 0.5f;
	float actionCoolDownTimer = 0.0f;
	Action action = Action::IDLE;

	PROPERTY(float, speed, 1.0f);
};
REGISTER_CLASS(FirstPersonCharacter);

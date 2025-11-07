#pragma once

#include "Core/ReflectionSystem.h"
#include "Graphics/SkeletalAnimation.h"
#include "../FirstPersonExample/StateMachine.h"

namespace JPH
{
	class Character;
}

struct FirstPersonCharacter
{
	std::map<State, std::shared_ptr<Pengine::SkeletalAnimation>> animations;
	std::map<State, std::shared_ptr<CharacterState>> states;
	
	std::shared_ptr<class JPH::Character> joltCharacter;
	float height = 1.0f;
	float radius = 0.5f;

	float shotTimer = 0.0f;

	std::string ammoUI;

	CharacterState* currentState;

	bool isJumpPressed = false;
	bool isMovePressed = false;
	bool isShotPressed = false;

	public: PROPERTY(int, currentAmmo, 90);
	public: PROPERTY(int, currentMagazine, 30);
	public: PROPERTY(int, maxMagazine, 30);
	public: PROPERTY(float, speed, 1.0f);
	public: PROPERTY(float, jump, 1.0f);
	public: PROPERTY(float, bulletForce, 100.0f);
	public: PROPERTY(float, damage, 30.0f);
};
REGISTER_CLASS(FirstPersonCharacter);

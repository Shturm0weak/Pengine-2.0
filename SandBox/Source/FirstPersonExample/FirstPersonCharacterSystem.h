#pragma once

#include "Core/Core.h"

#include "ComponentSystems/ComponentSystem.h"

#include "FirstPersonCharacter.h"

class PENGINE_API FirstPersonCharacterSystem : public Pengine::ComponentSystem
{
public:
	FirstPersonCharacterSystem();
	virtual ~FirstPersonCharacterSystem() override = default;

	virtual void OnUpdate(const float deltaTime, std::shared_ptr<class Pengine::Scene> scene) override;

	virtual void OnPrePhysicsUpdate(const float deltaTime, std::shared_ptr<class Pengine::Scene> scene) override;

	virtual void OnPostPhysicsUpdate(const float deltaTime, std::shared_ptr<class Pengine::Scene> scene) override;
//
//private:
//	struct InputState
//	{
//		glm::vec3 direction;
//		bool isJumping;
//		bool isSprinting;
//	};
//
//	class State
//	{
//	public:
//		virtual void OnEnter() {}
//		virtual void OnUpdate(float deltaTime, InputState& inputState, entt::entity handle, entt::registry& registry) {}
//		virtual void OnExit() {}
//	};
//
//	class IdleState : public State
//	{
//	};
//
//	class WalkState : public State
//	{
//	};
//
//	std::unordered_map<FirstPersonCharacter::Action, std::shared_ptr<State>> m_StatesByAction;
//
//	void TransitionTo(std::shared_ptr<State> currentState, std::shared_ptr<State> nextState);
};

#include "CharacterControllerExample.h"

#include "Core/Serializer.h"

#include "Core/Scene.h"
#include "Core/Time.h"
#include "Core/Input.h"
#include "Core/KeyCode.h"
#include "Core/WindowManager.h"

#include "Components/Transform.h"

#include "ComponentSystems/PhysicsSystem.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

using namespace Pengine;

void CharacterControllerExample::OnPreStart()
{
}

void HandleInput(JPH::Character* mCharacter, JPH::Vec3Arg inMovementDirection, bool inJump, float inDeltaTime)
{
	bool sControlMovementDuringJump = true;
	float sCharacterSpeed = 5.0f;
	float sJumpSpeed = 10.0f;

	using namespace JPH;
	// Cancel movement in opposite direction of normal when touching something we can't walk up
	Vec3 movement_direction = inMovementDirection;
	Character::EGroundState ground_state = mCharacter->GetGroundState();
	if (ground_state == Character::EGroundState::OnSteepGround
		|| ground_state == Character::EGroundState::NotSupported)
	{
		Vec3 normal = mCharacter->GetGroundNormal();
		normal.SetY(0.0f);
		float dot = normal.Dot(movement_direction);
		if (dot < 0.0f)
			movement_direction -= (dot * normal) / normal.LengthSq();
	}

	if (sControlMovementDuringJump || mCharacter->IsSupported())
	{
		// Update velocity
		Vec3 current_velocity = mCharacter->GetLinearVelocity();
		Vec3 desired_velocity = sCharacterSpeed * movement_direction;
		if (!desired_velocity.IsNearZero() || current_velocity.GetY() < 0.0f || !mCharacter->IsSupported())
			desired_velocity.SetY(current_velocity.GetY());
		Vec3 new_velocity = 0.75f * current_velocity + 0.25f * desired_velocity;

		// Jump
		if (inJump && ground_state == Character::EGroundState::OnGround)
			new_velocity += Vec3(0, sJumpSpeed, 0);

		// Update the velocity
		mCharacter->SetLinearVelocity(new_velocity);
	}
}


void CharacterControllerExample::OnStart()
{
	m_Scene = Serializer::DeserializeScene("Scenes/Examples/CharacterController/CharacterController.scene");

	m_Character.entity = m_Scene->FindEntityByName("Character");
	Transform& transform = m_Character.entity->GetComponent<Transform>();
	JPH::CharacterSettings settings;
	settings.mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
	settings.mLayer = ObjectLayers::DYNAMIC;
	settings.mShape = JPH::RotatedTranslatedShapeSettings(
		JPH::Vec3(0, 0, 0),
		JPH::Quat::sIdentity(),
		new JPH::CapsuleShape(0.5f * m_Character.height, m_Character.radius)).Create().Get();

	settings.mFriction = 0.5f;
	settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -m_Character.radius);

	m_Character.joltCharacter = std::make_unique<JPH::Character>(
		&settings,
		GlmVec3ToJoltVec3(transform.GetPosition()),
		GlmQuatToJoltQuat(transform.GetRotation()),
		0,
		&std::static_pointer_cast<PhysicsSystem>(m_Scene->GetComponentSystem("PhysicsSystem"))->GetInstance());
	m_Character.joltCharacter->AddToPhysicsSystem();
}

void CharacterControllerExample::OnUpdate()
{
	const auto physicsSystem = std::static_pointer_cast<PhysicsSystem>(m_Scene->GetComponentSystem("PhysicsSystem"));
	auto& joltPhysicsSystem = physicsSystem->GetInstance();

	/*m_Character.joltCharacter->UpdateGroundVelocity();
	m_Character.joltCharacter->Update(Time::GetDeltaTime(),
		joltPhysicsSystem.GetGravity(),
		joltPhysicsSystem.GetDefaultBroadPhaseLayerFilter(ObjectLayers::DYNAMIC),
		joltPhysicsSystem.GetDefaultLayerFilter(ObjectLayers::DYNAMIC),
		{},
		{},
		*physicsSystem->GetTempAllocator());*/

	Transform& transform = m_Character.entity->GetComponent<Transform>();
	transform.Translate(JoltVec3ToGlmVec3(m_Character.joltCharacter->GetPosition()));
	transform.Rotate(glm::eulerAngles(JoltQuatToGlmQuat(m_Character.joltCharacter->GetRotation())));

	auto& input = Input::GetInstance(WindowManager::GetInstance().GetWindowByName("Main").get());

	JPH::Vec3 direction{};
	if (input.IsKeyDown(KeyCode::KEY_W))
	{
		direction.SetX(1.0f);
	}
	if (input.IsKeyDown(KeyCode::KEY_S))
	{
		direction.SetX(-1.0f);
	}
	if (input.IsKeyDown(KeyCode::KEY_D))
	{
		direction.SetZ(1.0f);
	}
	if (input.IsKeyDown(KeyCode::KEY_A))
	{
		direction.SetZ(-1.0f);
	}

	bool jump = input.IsKeyPressed(KeyCode::SPACE);

	HandleInput(m_Character.joltCharacter.get(), direction.NormalizedOr({}), jump, Time::GetDeltaTime());

	m_Character.joltCharacter->PostSimulation(0.1f);
}

void CharacterControllerExample::OnClose()
{
	m_Character.joltCharacter->RemoveFromPhysicsSystem();
	m_Character.joltCharacter = nullptr;
	m_Scene = nullptr;
}
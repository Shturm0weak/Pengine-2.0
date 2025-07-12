#include "EntityAnimatorSystem.h"

#include "../Components/EntityAnimator.h"
#include "../Components/Transform.h"
#include "../Core/Scene.h"

using namespace Pengine;

EntityAnimator::Keyframe InterpolateLinear(
	const EntityAnimator::Keyframe& a,
	const EntityAnimator::Keyframe& b,
	float t)
{
	EntityAnimator::Keyframe keyframe{};
	keyframe.time = glm::mix(a.time, b.time, t);
	keyframe.translation = glm::mix(a.translation, b.translation, t);
	keyframe.rotation = glm::mix(a.rotation, b.rotation, t);
	keyframe.scale = glm::mix(a.scale, b.scale, t);
	return keyframe;
}

EntityAnimator::Keyframe InterpolateBezier(
	const EntityAnimator::Keyframe& a,
	const EntityAnimator::Keyframe& b,
	float t)
{
	EntityAnimator::Keyframe keyframe{};
	const float t2 = t * t;
	const float t3 = t2 * t;
	const float invT = 1.0f - t;
	const float invT2 = invT * invT;
	const float invT3 = invT2 * invT;

	auto cubic = [](float p0, float p1, float t)
	{
		return p0 * (1.0f - t) * (1.0f - t) * (1.0f - t) +
			p1 * t * t * t;
	};

	keyframe.time = glm::mix(a.time, b.time, t);
	keyframe.translation = glm::vec3(
		cubic(a.translation.x, b.translation.x, t),
		cubic(a.translation.y, b.translation.y, t),
		cubic(a.translation.z, b.translation.z, t));
	keyframe.rotation = glm::mix(a.rotation, b.rotation, t);
	keyframe.scale = glm::vec3(
		cubic(a.scale.x, b.scale.x, t),
		cubic(a.scale.y, b.scale.y, t),
		cubic(a.scale.z, b.scale.z, t));

	return keyframe;
}

EntityAnimator::Keyframe GetInterpolatedKeyframe(
	const EntityAnimator::AnimationTrack& animationTrack,
	float time)
{
	if (animationTrack.keyframes.empty())
	{
		return {};
	}

	if (time <= animationTrack.keyframes.front().time)
	{
		return animationTrack.keyframes.front();
	}

	if (time >= animationTrack.keyframes.back().time)
	{
		return animationTrack.keyframes.back();
	}

	size_t prevIndex = 0;
	size_t nextIndex = 0;
	for (size_t i = 0; i < animationTrack.keyframes.size() - 1; i++)
	{
		if (time >= animationTrack.keyframes[i].time && time <= animationTrack.keyframes[i + 1].time)
		{
			prevIndex = i;
			nextIndex = i + 1;
			break;
		}
	}

	const auto& prevKey = animationTrack.keyframes[prevIndex];
	const auto& nextKey = animationTrack.keyframes[nextIndex];
	float t = (time - prevKey.time) / (nextKey.time - prevKey.time);

	switch (prevKey.interpType)
	{
	case EntityAnimator::Keyframe::InterpolationType::STEP:
		return prevKey;

	case EntityAnimator::Keyframe::InterpolationType::LINEAR:
		return InterpolateLinear(prevKey, nextKey, t);

	case EntityAnimator::Keyframe::InterpolationType::BEZIER:
		return InterpolateBezier(prevKey, nextKey, t);
	}

	return {};
}

void EntityAnimatorSystem::OnUpdate(const float deltaTime, std::shared_ptr<Scene> scene)
{
	const auto& view = scene->GetRegistry().view<EntityAnimator>();
	for (const entt::entity entity : view)
	{
		EntityAnimator& entityAnimator = scene->GetRegistry().get<EntityAnimator>(entity);

		if (!entityAnimator.animationTrack || entityAnimator.animationTrack->keyframes.empty() || !entityAnimator.isPlaying)
		{
			continue;
		}

		Transform& transform = scene->GetRegistry().get<Transform>(entity);

		entityAnimator.time += deltaTime * entityAnimator.speed;
		EntityAnimator::Keyframe keyframe = GetInterpolatedKeyframe(*entityAnimator.animationTrack, entityAnimator.time);

		transform.Translate(keyframe.translation);
		transform.Rotate(keyframe.rotation);
		transform.Scale(keyframe.scale);

		if (entityAnimator.isLoop)
		{
			if (entityAnimator.time >= entityAnimator.animationTrack->keyframes.back().time)
			{
				entityAnimator.time = 0;
			}
		}
	}
}

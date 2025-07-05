#include "SkeletalAnimatorSystem.h"

#include "../Components/SkeletalAnimator.h"
#include "../Components/Transform.h"
#include "../Core/Scene.h"

using namespace Pengine;

void SkeletalAnimatorSystem::OnUpdate(const float deltaTime, std::shared_ptr<Scene> scene)
{
	const auto& view = scene->GetRegistry().view<SkeletalAnimator>();
	for (const entt::entity entity : view)
	{
		SkeletalAnimator& skeletalAnimator = scene->GetRegistry().get<SkeletalAnimator>(entity);
		Transform& transform = scene->GetRegistry().get<Transform>(entity);
		skeletalAnimator.UpdateAnimation(transform.GetEntity()->GetTopEntity(), deltaTime, glm::mat4(1.0f));
	}
}

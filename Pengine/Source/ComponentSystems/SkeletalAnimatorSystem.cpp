#include "SkeletalAnimatorSystem.h"

#include "../Components/SkeletalAnimator.h"
#include "../Components/Transform.h"
#include "../Core/Scene.h"
#include "../Core/ThreadPool.h"
#include "../Core/Timer.h"

#include <future>

using namespace Pengine;

void SkeletalAnimatorSystem::OnUpdate(const float deltaTime, std::shared_ptr<Scene> scene)
{
	const auto& view = scene->GetRegistry().view<SkeletalAnimator>();
	if (view.empty())
	{
		return;
	}

	std::vector<std::future<void>> futures;
	glm::mat4 identity = glm::mat4(1.0f);
	for (const entt::entity entity : view)
	{
		futures.emplace_back(ThreadPool::GetInstance().EnqueueAsyncFuture([this, scene, entity, &identity, deltaTime]()
		{
			SkeletalAnimator& skeletalAnimator = scene->GetRegistry().get<SkeletalAnimator>(entity);
			Transform& transform = scene->GetRegistry().get<Transform>(entity);
			std::shared_ptr<Entity> topEntity = transform.GetEntity()->GetTopEntity();
			skeletalAnimator.UpdateAnimation(topEntity, deltaTime, identity);
		}));
	}

	for (auto& future : futures)
	{
		future.get();
	}
}

#include "SkeletalAnimatorSystem.h"

#include "../Components/SkeletalAnimator.h"
#include "../Components/Transform.h"
#include "../Core/Scene.h"
#include "../Core/ThreadPool.h"
#include "../Core/Timer.h"

#include <barrier>

using namespace Pengine;

void SkeletalAnimatorSystem::OnUpdate(const float deltaTime, std::shared_ptr<Scene> scene)
{
	const auto& view = scene->GetRegistry().view<SkeletalAnimator>();
	if (view.empty())
	{
		return;
	}

	std::atomic<int> finishedCount = 0;
	std::atomic<bool> finishedAllTasks = false;
	glm::mat4 identity = glm::mat4(1.0f);
	const int skeletalAnimatorCount = view.size();
	for (const entt::entity entity : view)
	{
		ThreadPool::GetInstance().EnqueueAsync([this, &finishedCount, &finishedAllTasks, skeletalAnimatorCount, scene, entity, &identity, deltaTime]()
		{
			SkeletalAnimator& skeletalAnimator = scene->GetRegistry().get<SkeletalAnimator>(entity);
			Transform& transform = scene->GetRegistry().get<Transform>(entity);
			std::shared_ptr<Entity> topEntity = transform.GetEntity()->GetTopEntity();
			skeletalAnimator.UpdateAnimation(topEntity, deltaTime, identity);

			if (finishedCount.fetch_add(1) + 1 == skeletalAnimatorCount)
			{
				finishedAllTasks.store(true);
				finishedAllTasks.notify_one();
			}
		});
	}

	finishedAllTasks.wait(false);
}

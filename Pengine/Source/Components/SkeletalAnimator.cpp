#include "SkeletalAnimator.h"

#include "../Graphics/SkeletalAnimation.h"
#include "../Graphics/Skeleton.h"
#include "../Core/Logger.h"

namespace Pengine
{

	SkeletalAnimator::SkeletalAnimator()
	{
		m_FinalBoneMatrices.resize(100, glm::mat4(1.0f));
	}

	SkeletalAnimator::SkeletalAnimator(const SkeletalAnimator& skeletalAnimator)
	{
		m_CurrentTime = skeletalAnimator.GetCurrentTime();
		m_Speed = skeletalAnimator.GetSpeed();
		m_SkeletalAnimation = skeletalAnimator.GetSkeletalAnimation();
		m_Skeleton = skeletalAnimator.GetSkeleton();
		m_FinalBoneMatrices.resize(100, glm::mat4(1.0f));

		// For every new skeletal animator component need to create a unique uniform writer and buffer.
		m_UniformWriter = nullptr;
		m_Buffer = nullptr;
	}

	void SkeletalAnimator::UpdateAnimation(const float deltaTime, const glm::mat4& parentTransform)
	{
		if (!m_SkeletalAnimation || !m_Skeleton)
		{
			return;
		}

		m_CurrentTime += m_SkeletalAnimation->GetTicksPerSecond() * deltaTime * m_Speed;
		m_CurrentTime = fmod(m_CurrentTime, m_SkeletalAnimation->GetDuration());
		CalculateBoneTransform(m_Skeleton->GetRootBoneId(), parentTransform);
	}

	void SkeletalAnimator::CalculateBoneTransform(const uint32_t boneId, const glm::mat4& parentTransform)
	{
		const Skeleton::Bone& node = m_Skeleton->GetBones()[boneId];
		std::string nodeName = node.name;
		glm::mat4 nodeTransform = node.transform;
		glm::mat4 globalTransformation = parentTransform * nodeTransform;
		if (m_SkeletalAnimation->GetBonesByName().contains(nodeName))
		{
			const SkeletalAnimation::Bone& bone = m_SkeletalAnimation->GetBonesByName().at(nodeName);
			nodeTransform = bone.Update(m_CurrentTime);
			globalTransformation = parentTransform * nodeTransform;
			m_FinalBoneMatrices[node.id] = globalTransformation * node.offset;
		}

		for (int i = 0; i < node.childIds.size(); i++)
		{
			CalculateBoneTransform(m_Skeleton->GetBones()[node.childIds[i]].id, globalTransformation);
		}
	}

}

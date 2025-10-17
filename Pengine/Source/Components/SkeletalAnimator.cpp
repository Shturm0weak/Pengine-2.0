#include "SkeletalAnimator.h"

#include "../Graphics/SkeletalAnimation.h"
#include "../Graphics/Skeleton.h"
#include "../Components/Transform.h"
#include "../Core/Logger.h"
#include "../Core/Entity.h"
#include "../Core/Scene.h"
#include "../Utils/Utils.h"

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

	void SkeletalAnimator::UpdateAnimation(std::shared_ptr<Entity> entity, const float deltaTime, const glm::mat4& parentTransform)
	{
		if (!m_SkeletalAnimation || !m_Skeleton)
		{
			return;
		}

		m_CurrentTime += deltaTime * m_Speed;
		
		if (m_NextSkeletalAnimation)
		{
			m_NextTime += deltaTime * m_Speed;
			if (!m_IsBlending)
			{
				m_TransitionTimer += deltaTime;

				if (m_TransitionTimer >= m_TransitionTime)
				{
					m_TransitionTimer = 0.0f;
					m_TransitionTime = 0.0f;
					m_CurrentTime = m_NextTime;
					m_NextTime = 0.0f;
					m_SkeletalAnimation = m_NextSkeletalAnimation;
					m_NextSkeletalAnimation = nullptr;
				}
			}
		}

		m_CurrentTime = fmod(m_CurrentTime, m_SkeletalAnimation->GetDuration());
		if (m_NextSkeletalAnimation)
		{
			m_NextTime = fmod(m_NextTime, m_NextSkeletalAnimation->GetDuration());
		}

		for (const uint32_t& rootBoneId : m_Skeleton->GetRootBoneIds())
		{
			CalculateBoneTransform(entity, rootBoneId, parentTransform);
		}
	}

	void SkeletalAnimator::SetNextSkeletalAnimation(std::shared_ptr<SkeletalAnimation> skeletalAnimation, float transitionTime)
	{
		m_NextSkeletalAnimation = skeletalAnimation;
		m_TransitionTime = transitionTime;
		m_TransitionTimer = 0.0f;
		m_NextTime = 0.0f;
		m_IsBlending = false;
	}

	void SkeletalAnimator::BlendSkeletalAnimations(
		std::shared_ptr<SkeletalAnimation> firstSkeletalAnimation,
		std::shared_ptr<SkeletalAnimation> secondSkeletalAnimation,
		float value)
	{
		m_SkeletalAnimation = firstSkeletalAnimation;
		m_NextSkeletalAnimation = secondSkeletalAnimation;
		m_TransitionTime = 1.0f;
		m_TransitionTimer = value;
		m_IsBlending = true;
	}

	void SkeletalAnimator::CalculateBoneTransform(std::shared_ptr<Entity> entity, const uint32_t boneId, const glm::mat4& parentTransform)
	{
		const Skeleton::Bone& node = m_Skeleton->GetBones()[boneId];
		std::string nodeName = node.name;
		glm::mat4 nodeTransform = node.transform;
		glm::mat4 globalTransformation = parentTransform * nodeTransform;

		glm::vec3 currentPosition;
		glm::quat currentRotation;
		glm::vec3 currentScale;
		if (m_SkeletalAnimation->GetBonesByName().contains(nodeName))
		{
			const SkeletalAnimation::Bone& bone = m_SkeletalAnimation->GetBonesByName().at(nodeName);
			bone.Update(m_CurrentTime, currentPosition, currentRotation, currentScale);
		}

		if (m_NextSkeletalAnimation && m_NextSkeletalAnimation->GetBonesByName().contains(nodeName))
		{
			const SkeletalAnimation::Bone& bone = m_NextSkeletalAnimation->GetBonesByName().at(nodeName);

			glm::vec3 nextPosition;
			glm::quat nextRotation;
			glm::vec3 nextScale;
			bone.Update(m_NextTime, nextPosition, nextRotation, nextScale);

			currentPosition = glm::mix(currentPosition, nextPosition, m_TransitionTimer / m_TransitionTime);
			currentRotation = glm::normalize(glm::slerp(currentRotation, nextRotation, m_TransitionTimer / m_TransitionTime));
			currentScale = glm::mix(currentScale, nextScale, m_TransitionTimer / m_TransitionTime);
		}

		nodeTransform = glm::translate(glm::mat4(1.0f), currentPosition) * glm::toMat4(currentRotation) * glm::scale(glm::mat4(1.0f), currentScale);
		globalTransformation = parentTransform * nodeTransform;
		m_FinalBoneMatrices[node.id] = globalTransformation * node.offset;

		// TODO: Very slow, maybe optimize!
		if (const auto& boneEntity = entity->FindEntityInHierarchy(node.name))
		{
			Transform& boneEntityTransform = boneEntity->GetComponent<Transform>();
			boneEntityTransform.Translate(currentPosition);
			boneEntityTransform.Rotate(glm::eulerAngles(currentRotation));
			boneEntityTransform.Scale(currentScale);
			if (GetDrawDebugSkeleton() && boneEntity->HasParent())
			{
				boneEntity->GetScene()->GetVisualizer().DrawLine(
					boneEntityTransform.GetPosition(),
					boneEntity->GetParent()->GetComponent<Transform>().GetPosition(),
					{ 1.0f, 0.0f, 1.0f });
			}
		}

		for (int i = 0; i < node.childIds.size(); i++)
		{
			CalculateBoneTransform(entity, m_Skeleton->GetBones()[node.childIds[i]].id, globalTransformation);
		}
	}

}

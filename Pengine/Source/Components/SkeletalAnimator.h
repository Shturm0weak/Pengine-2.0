#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class Skeleton;
	class SkeletalAnimation;
	class UniformWriter;
	class Buffer;

	class PENGINE_API SkeletalAnimator
	{
	public:
		SkeletalAnimator();
		SkeletalAnimator(const SkeletalAnimator& skeletalAnimator);

		void UpdateAnimation(const float deltaTime, const glm::mat4& parentTransform);

		[[nodiscard]] float GetSpeed() const { return m_Speed; }

		void SetSpeed(const float speed) { m_Speed = speed; }

		[[nodiscard]] float GetCurrentTime() const { return m_CurrentTime; }

		void SetCurrentTime(const float currentTime) { m_CurrentTime = currentTime; }

		[[nodiscard]] const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return m_FinalBoneMatrices; }

		[[nodiscard]] std::shared_ptr<UniformWriter> GetUniformWriter() const { return m_UniformWriter; }
		
		void SetUniformWriter(std::shared_ptr<UniformWriter> uniformWriter) { m_UniformWriter = uniformWriter; }

		[[nodiscard]] std::shared_ptr<Buffer> GetBuffer() const { return m_Buffer; }

		void SetBuffer(std::shared_ptr<Buffer> buffer) { m_Buffer = buffer; }

		[[nodiscard]] std::shared_ptr<Skeleton> GetSkeleton() const { return m_Skeleton; }

		void SetSkeleton(std::shared_ptr<Skeleton> skeleton) { m_Skeleton = skeleton; }

		[[nodiscard]] std::shared_ptr<SkeletalAnimation> GetSkeletalAnimation() const { return m_SkeletalAnimation; }

		void SetSkeletalAnimation(std::shared_ptr<SkeletalAnimation> skeletalAnimation) { m_SkeletalAnimation = skeletalAnimation; }

	private:
		void CalculateBoneTransform(const uint32_t boneId, const glm::mat4& parentTransform);

		std::vector<glm::mat4> m_FinalBoneMatrices;

		std::shared_ptr<Skeleton> m_Skeleton;
		std::shared_ptr<SkeletalAnimation> m_SkeletalAnimation;

		std::shared_ptr<UniformWriter> m_UniformWriter;
		std::shared_ptr<Buffer> m_Buffer;

		float m_Speed = 0.0f;
		float m_CurrentTime = 0.0f;		
	};

}

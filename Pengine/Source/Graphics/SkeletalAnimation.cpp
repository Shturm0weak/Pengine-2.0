#include "SkeletalAnimation.h"

namespace Pengine
{

	glm::mat4 SkeletalAnimation::Bone::Update(float time) const
	{
		glm::mat4 translation = InterpolatePosition(time);
		glm::mat4 rotation = InterpolateRotation(time);
		glm::mat4 scale = InterpolateScaling(time);
		return translation * rotation * scale;
	}

	int SkeletalAnimation::Bone::GetPositionIndex(float animationTime) const
	{
		for (int index = 0; index < positions.size() - 1; ++index)
		{
			if (animationTime < positions[index + 1].time)
			{
				return index;
			}
		}
		assert(0);
	}

	int SkeletalAnimation::Bone::GetRotationIndex(float animationTime) const
	{
		for (int index = 0; index < rotations.size() - 1; ++index)
		{
			if (animationTime < rotations[index + 1].time)
			{
				return index;
			}
		}
		assert(0);
	}

	int SkeletalAnimation::Bone::GetScaleIndex(float animationTime) const
	{
		for (int index = 0; index < scales.size() - 1; ++index)
		{
			if (animationTime < scales[index + 1].time)
			{
				return index;
			}
		}
		assert(0);
	}

	float SkeletalAnimation::Bone::GetScaleFactor(float lastTime, float nextTime, float time) const
	{
		return (time - lastTime) / (nextTime - lastTime);
	}

	glm::mat4 SkeletalAnimation::Bone::InterpolatePosition(float time) const
	{
		if (positions.size() == 1)
		{
			return glm::translate(glm::mat4(1.0f), positions[0].value);
		}

		const int p0Index = GetPositionIndex(time);
		const int p1Index = p0Index + 1;
		const float scaleFactor = GetScaleFactor(positions[p0Index].time, positions[p1Index].time, time);
		glm::vec3 position = glm::mix(positions[p0Index].value, positions[p1Index].value, scaleFactor);

		return glm::translate(glm::mat4(1.0f), position);
	}

	glm::mat4 SkeletalAnimation::Bone::InterpolateRotation(float time) const
	{
		if (rotations.size() == 1)
		{
			const glm::quat rotation = glm::normalize(rotations[0].value);
			return glm::toMat4(rotation);
		}

		const int p0Index = GetRotationIndex(time);
		const int p1Index = p0Index + 1;
		const float scaleFactor = GetScaleFactor(rotations[p0Index].time, rotations[p1Index].time, time);
		const glm::quat rotation = glm::slerp(rotations[p0Index].value, rotations[p1Index].value, scaleFactor);
		return glm::toMat4(glm::normalize(rotation));
	}

	glm::mat4 SkeletalAnimation::Bone::InterpolateScaling(float time) const
	{
		if (scales.size() == 1)
		{
			return glm::scale(glm::mat4(1.0f), scales[0].value);
		}

		const int p0Index = GetScaleIndex(time);
		const int p1Index = p0Index + 1;
		const float scaleFactor = GetScaleFactor(scales[p0Index].time, scales[p1Index].time, time);
		const glm::vec3 scale = glm::mix(scales[p0Index].value, scales[p1Index].value, scaleFactor);
		return glm::scale(glm::mat4(1.0f), scale);
	}

}

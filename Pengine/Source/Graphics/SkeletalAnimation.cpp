#include "SkeletalAnimation.h"

namespace Pengine
{

	glm::mat4 SkeletalAnimation::Bone::Update(float time) const
	{
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
		Update(time, position, rotation, scale);

		return glm::translate(glm::mat4(1.0f), position) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
	}

	void SkeletalAnimation::Bone::Update(float time, glm::vec3& position, glm::quat& rotation, glm::vec3& scale) const
	{
		position = InterpolatePosition(time);
		rotation = InterpolateRotation(time);
		scale = InterpolateScaling(time);
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

	glm::vec3 SkeletalAnimation::Bone::InterpolatePosition(float time) const
	{
		if (positions.empty())
		{
			return glm::vec3(0.0f);
		}

		if (positions.size() == 1)
		{
			return positions[0].value;
		}

		const int p0Index = GetPositionIndex(time);
		const int p1Index = p0Index + 1;
		const float scaleFactor = GetScaleFactor(positions[p0Index].time, positions[p1Index].time, time);
		return glm::mix(positions[p0Index].value, positions[p1Index].value, scaleFactor);
	}

	glm::quat SkeletalAnimation::Bone::InterpolateRotation(float time) const
	{
		if (rotations.empty())
		{
			return glm::quat(glm::vec3(0.0f));
		}

		if (rotations.size() == 1)
		{
			return glm::normalize(rotations[0].value);
		}

		const int p0Index = GetRotationIndex(time);
		const int p1Index = p0Index + 1;
		const float scaleFactor = GetScaleFactor(rotations[p0Index].time, rotations[p1Index].time, time);
		return glm::normalize(glm::slerp(rotations[p0Index].value, rotations[p1Index].value, scaleFactor));
	}

	glm::vec3 SkeletalAnimation::Bone::InterpolateScaling(float time) const
	{
		if (scales.empty())
		{
			return glm::vec3(1.0f);
		}

		if (scales.size() == 1)
		{
			return scales[0].value;
		}

		const int p0Index = GetScaleIndex(time);
		const int p1Index = p0Index + 1;
		const float scaleFactor = GetScaleFactor(scales[p0Index].time, scales[p1Index].time, time);
		return glm::mix(scales[p0Index].value, scales[p1Index].value, scaleFactor);
	}

}

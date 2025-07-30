#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

namespace Pengine
{

	class PENGINE_API SkeletalAnimation final : public Asset
	{
	public:
		struct KeyVec
		{
			double time;
			glm::vec3 value;
		};

		struct KeyQuat
		{
			double time;
			glm::quat value;
		};

		class Bone
		{
		public:
			std::vector<KeyVec> positions;
			std::vector<KeyQuat> rotations;
			std::vector<KeyVec> scales;

			glm::mat4 Update(float time) const;

			void Update(float time, glm::vec3& position, glm::quat& rotation, glm::vec3& scale) const;

			int GetPositionIndex(float animationTime) const;

			int GetRotationIndex(float animationTime) const;

			int GetScaleIndex(float animationTime) const;

		private:
			float GetScaleFactor(float lastTime, float nextTime, float time) const;

			glm::vec3 InterpolatePosition(float time) const;

			glm::quat InterpolateRotation(float time) const;

			glm::vec3 InterpolateScaling(float time) const;
		};

		struct CreateInfo
		{
			std::string name;
			std::filesystem::path filepath;
			double duration;
			std::unordered_map<std::string, Bone> bonesByName;
		};

		SkeletalAnimation(const CreateInfo& createInfo)
			: Asset(createInfo.name, createInfo.filepath)
			, m_Duration(createInfo.duration)
			, m_BonesByName(createInfo.bonesByName)
		{
		}

		[[nodiscard]] double GetDuration() const { return m_Duration; }

		[[nodiscard]] const std::unordered_map<std::string, Bone>& GetBonesByName() const { return m_BonesByName; }

	private:
		double m_Duration;

		std::unordered_map<std::string, Bone> m_BonesByName;
	};

}

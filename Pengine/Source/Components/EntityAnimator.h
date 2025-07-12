#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

namespace Pengine
{

	class PENGINE_API EntityAnimator
	{
	public:
		struct Keyframe
		{
			float time;
			glm::vec3 translation;
			glm::vec3 rotation;
			glm::vec3 scale;
			bool selected = false;

			enum InterpolationType { LINEAR, BEZIER, STEP };
			InterpolationType interpType = LINEAR;
		};

		class AnimationTrack : public Asset
		{
		public:
			AnimationTrack(
				const std::string& name,
				const std::filesystem::path& filepath)
				: Asset(name, filepath)
			{
			}

			std::vector<Keyframe> keyframes;
			bool visible = true;
		};

		std::optional<AnimationTrack> animationTrack;
		float time = 0.0f;
		float speed = 1.0f;
		bool isPlaying = true;
		bool isLoop = true;
	};

}

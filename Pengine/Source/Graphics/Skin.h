#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

namespace Pengine
{

	class PENGINE_API Skin final : public Asset
	{
	public:

		struct CreateInfo
		{
			std::string name;
			std::filesystem::path filepath;

			std::vector<glm::vec4> weights;
			std::vector<glm::ivec4> boneIds;
		};

		Skin(const CreateInfo& createInfo)
			: Asset(createInfo.name, createInfo.filepath)
			, m_Weights(createInfo.weights)
			, m_BoneIds(createInfo.boneIds)
		{
		}

		[[nodiscard]] const std::vector<glm::vec4>& GetWeights() const { return m_Weights; }

		[[nodiscard]] const std::vector<glm::ivec4>& GetBoneIds() const { return m_BoneIds; }

	private:
		std::vector<glm::vec4> m_Weights;
		std::vector<glm::ivec4> m_BoneIds;
	};

}

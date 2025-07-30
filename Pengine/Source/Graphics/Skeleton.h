#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

namespace Pengine
{

	class PENGINE_API Skeleton final : public Asset
	{
	public:
		struct Bone
		{
			std::string name;
			glm::mat4 transform;
			glm::mat4 offset;

			uint32_t id = -1;
			uint32_t parentId = -1;
			std::vector<uint32_t> childIds;
		};

		struct CreateInfo
		{
			std::string name;
			std::filesystem::path filepath;
			std::vector<Bone> bones;
			std::vector<uint32_t> rootBoneIds;
		};

		Skeleton(const CreateInfo& createInfo)
			: Asset(createInfo.name, createInfo.filepath)
			, m_RootBoneIds(createInfo.rootBoneIds)
			, m_Bones(createInfo.bones)
		{
		}

		[[nodiscard]] const std::vector<uint32_t>& GetRootBoneIds() const { return m_RootBoneIds; }

		[[nodiscard]] const std::vector<Bone>& GetBones() const { return m_Bones; }

	private:

		std::vector<uint32_t> m_RootBoneIds;
		std::vector<Bone> m_Bones;
	};

}

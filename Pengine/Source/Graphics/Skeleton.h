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
			uint32_t rootBoneId = -1;
			std::vector<Bone> bones;
		};

		Skeleton(const CreateInfo& createInfo)
			: Asset(createInfo.name, createInfo.filepath)
			, m_RootBoneId(createInfo.rootBoneId)
			, m_Bones(createInfo.bones)
		{
		}

		[[nodiscard]] uint32_t GetRootBoneId() const { return m_RootBoneId; }

		[[nodiscard]] const std::vector<Bone>& GetBones() const { return m_Bones; }

	private:

		uint32_t m_RootBoneId;
		std::vector<Bone> m_Bones;
	};

}

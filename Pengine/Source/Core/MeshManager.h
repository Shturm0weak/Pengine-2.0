#pragma once

#include "Core.h"

#include "../Graphics/Mesh.h"
#include "../Graphics/SkeletalAnimation.h"
#include "../Graphics/Skeleton.h"

#include <mutex>

namespace Pengine
{

	class PENGINE_API MeshManager
	{
	public:
		static MeshManager& GetInstance();

		MeshManager(const MeshManager&) = delete;
		MeshManager& operator=(const MeshManager&) = delete;

		std::shared_ptr<Mesh> CreateMesh(Mesh::CreateInfo& createInfo, const bool tryToGet = true);

		std::shared_ptr<Mesh> LoadMesh(const std::filesystem::path& filepath);

		std::shared_ptr<Mesh> GetMesh(const std::filesystem::path& filepath) const;

		void DeleteMesh(std::shared_ptr<Mesh>& mesh);

		std::shared_ptr<SkeletalAnimation> CreateSkeletalAnimation(SkeletalAnimation::CreateInfo& createInfo);

		std::shared_ptr<SkeletalAnimation> LoadSkeletalAnimation(const std::filesystem::path& filepath);

		std::shared_ptr<SkeletalAnimation> GetSkeletalAnimation(const std::filesystem::path& filepath) const;

		void DeleteSkeletalAnimation(std::shared_ptr<SkeletalAnimation>& skeletalAnimation);

		std::shared_ptr<Skeleton> CreateSkeleton(Skeleton::CreateInfo& createInfo);

		std::shared_ptr<Skeleton> LoadSkeleton(const std::filesystem::path& filepath);

		std::shared_ptr<Skeleton> GetSkeleton(const std::filesystem::path& filepath) const;

		void DeleteSkeleton(std::shared_ptr<Skeleton>& skeleton);

		const std::unordered_map<std::filesystem::path, std::shared_ptr<Mesh>, path_hash>& GetMeshes() const { return m_MeshesByFilepath; }

		const std::unordered_map<std::filesystem::path, std::shared_ptr<SkeletalAnimation>, path_hash>& GetSkeletalAnimations() const { return m_SkeletalAnimationsByFilepath; }

		const std::unordered_map<std::filesystem::path, std::shared_ptr<Skeleton>, path_hash>& GetSkeletons() const { return m_SkeletonsByFilepath; }

		void ShutDown();

		void ManipulateOnAllMaterialsDebug();

	private:
		MeshManager() = default;
		~MeshManager() = default;

		std::unordered_map<std::filesystem::path, std::shared_ptr<Mesh>, path_hash> m_MeshesByFilepath;
		std::unordered_map<std::filesystem::path, std::shared_ptr<Skeleton>, path_hash> m_SkeletonsByFilepath;
		std::unordered_map<std::filesystem::path, std::shared_ptr<SkeletalAnimation>, path_hash> m_SkeletalAnimationsByFilepath;

		mutable std::mutex m_MutexMesh;
		mutable std::mutex m_MutexSkeleton;
		mutable std::mutex m_MutexSkeletalAnimation;
	};

}
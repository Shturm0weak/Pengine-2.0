#include "MeshManager.h"

#include "TextureManager.h"
#include "MaterialManager.h"
#include "Logger.h"
#include "Serializer.h"

using namespace Pengine;

MeshManager& MeshManager::GetInstance()
{
	static MeshManager meshManager;
	return meshManager;
}

std::shared_ptr<Mesh> MeshManager::CreateMesh(Mesh::CreateInfo& createInfo)
{
	if (std::shared_ptr<Mesh> mesh = GetMesh(createInfo.filepath))
	{
		return mesh;
	}
	else
	{
		mesh = std::make_shared<Mesh>(createInfo);
		std::lock_guard<std::mutex> lock(m_MutexMesh);
		m_MeshesByFilepath[createInfo.filepath] = mesh;

		return mesh;
	}
}

std::shared_ptr<Mesh> MeshManager::LoadMesh(const std::filesystem::path& filepath)
{
	if (std::shared_ptr<Mesh> mesh = GetMesh(filepath))
	{
		return mesh;
	}
	else
	{
		mesh = Serializer::DeserializeMesh(filepath);
		if (!mesh)
		{
			FATAL_ERROR(filepath.string() + ":There is no such mesh!");
		}

		std::lock_guard<std::mutex> lock(m_MutexMesh);
		m_MeshesByFilepath.emplace(filepath, mesh);

		return mesh;
	}
}

std::shared_ptr<Mesh> MeshManager::GetMesh(const std::filesystem::path& filepath)
{
	std::lock_guard<std::mutex> lock(m_MutexMesh);
	auto meshByFilepath = m_MeshesByFilepath.find(filepath);
	if (meshByFilepath != m_MeshesByFilepath.end())
	{
		return meshByFilepath->second;
	}

	return nullptr;
}

void MeshManager::DeleteMesh(std::shared_ptr<Mesh> mesh)
{
	std::lock_guard<std::mutex> lock(m_MutexMesh);
	m_MeshesByFilepath.erase(mesh->GetFilepath());
}

std::shared_ptr<SkeletalAnimation> MeshManager::CreateSkeletalAnimation(SkeletalAnimation::CreateInfo& createInfo)
{
	if (std::shared_ptr<SkeletalAnimation> skeletalAnimation = GetSkeletalAnimation(createInfo.filepath))
	{
		return skeletalAnimation;
	}
	else
	{
		skeletalAnimation = std::make_shared<SkeletalAnimation>(createInfo);
		std::lock_guard<std::mutex> lock(m_MutexSkeletalAnimation);
		m_SkeletalAnimationsByFilepath[createInfo.filepath] = skeletalAnimation;

		return skeletalAnimation;
	}
}

std::shared_ptr<SkeletalAnimation> MeshManager::LoadSkeletalAnimation(const std::filesystem::path& filepath)
{
	if (std::shared_ptr<SkeletalAnimation> skeletalAnimation = GetSkeletalAnimation(filepath))
	{
		return skeletalAnimation;
	}
	else
	{
		skeletalAnimation = Serializer::DeserializeSkeletalAnimation(filepath);
		if (!skeletalAnimation)
		{
			FATAL_ERROR(filepath.string() + ":There is no such skeletal animation!");
		}

		std::lock_guard<std::mutex> lock(m_MutexSkeletalAnimation);
		m_SkeletalAnimationsByFilepath.emplace(filepath, skeletalAnimation);

		return skeletalAnimation;
	}
}

std::shared_ptr<SkeletalAnimation> MeshManager::GetSkeletalAnimation(const std::filesystem::path& filepath)
{
	std::lock_guard<std::mutex> lock(m_MutexSkeletalAnimation);
	auto skeletalAnimationsByFilepath = m_SkeletalAnimationsByFilepath.find(filepath);
	if (skeletalAnimationsByFilepath != m_SkeletalAnimationsByFilepath.end())
	{
		return skeletalAnimationsByFilepath->second;
	}

	return nullptr;
}

void MeshManager::DeleteSkeletalAnimation(std::shared_ptr<SkeletalAnimation> skeletalAnimation)
{
	std::lock_guard<std::mutex> lock(m_MutexSkeletalAnimation);
	m_SkeletalAnimationsByFilepath.erase(skeletalAnimation->GetFilepath());
}

std::shared_ptr<Skeleton> MeshManager::CreateSkeleton(Skeleton::CreateInfo& createInfo)
{
	if (std::shared_ptr<Skeleton> skeleton = GetSkeleton(createInfo.filepath))
	{
		return skeleton;
	}
	else
	{
		skeleton = std::make_shared<Skeleton>(createInfo);
		std::lock_guard<std::mutex> lock(m_MutexSkeleton);
		m_SkeletonsByFilepath[createInfo.filepath] = skeleton;

		return skeleton;
	}
}

std::shared_ptr<Skeleton> MeshManager::LoadSkeleton(const std::filesystem::path& filepath)
{
	if (std::shared_ptr<Skeleton> skeleton = GetSkeleton(filepath))
	{
		return skeleton;
	}
	else
	{
		skeleton = Serializer::DeserializeSkeleton(filepath);
		if (!skeleton)
		{
			FATAL_ERROR(filepath.string() + ":There is no such skeleton!");
		}

		std::lock_guard<std::mutex> lock(m_MutexSkeleton);
		m_SkeletonsByFilepath.emplace(filepath, skeleton);

		return skeleton;
	}
}

std::shared_ptr<Skeleton> MeshManager::GetSkeleton(const std::filesystem::path& filepath)
{
	std::lock_guard<std::mutex> lock(m_MutexSkeleton);
	auto skeletonsByFilepath = m_SkeletonsByFilepath.find(filepath);
	if (skeletonsByFilepath != m_SkeletonsByFilepath.end())
	{
		return skeletonsByFilepath->second;
	}

	return nullptr;
}

void MeshManager::DeleteSkeleton(std::shared_ptr<Skeleton> skeleton)
{
	std::lock_guard<std::mutex> lock(m_MutexSkeleton);
	m_SkeletonsByFilepath.erase(skeleton->GetFilepath());
}

void MeshManager::ShutDown()
{
	{
		std::lock_guard<std::mutex> lock(m_MutexMesh);
		m_MeshesByFilepath.clear();
	}

	{
		std::lock_guard<std::mutex> lock(m_MutexSkeletalAnimation);
		m_SkeletalAnimationsByFilepath.clear();
	}

	{
		std::lock_guard<std::mutex> lock(m_MutexSkeleton);
		m_SkeletonsByFilepath.clear();
	}
}

#include "SceneManager.h"

#include "MaterialManager.h"
#include "MeshManager.h"

#include "../Components/Renderer3D.h"

using namespace Pengine;

SceneManager& SceneManager::GetInstance()
{
	static SceneManager sceneManager;
	return sceneManager;
}

std::shared_ptr<Scene> SceneManager::Create(const std::string& name, const std::string& tag)
{
	std::shared_ptr<Scene> scene = std::make_shared<Scene>(name, none);
	scene->SetTag(tag);
	m_ScenesByName.emplace(name, scene);
	m_ScenesByTag.emplace(tag, scene);
	
	return scene;
}

std::shared_ptr<Scene> SceneManager::GetSceneByName(const std::string& name) const
{
	auto sceneByName = m_ScenesByName.find(name);
	if (sceneByName != m_ScenesByName.end())
	{
		return sceneByName->second;
	}

	return nullptr;
}

std::shared_ptr<Scene> SceneManager::GetSceneByTag(const std::string& tag) const
{
	auto sceneByTag = m_ScenesByTag.find(tag);
	if (sceneByTag != m_ScenesByTag.end())
	{
		return sceneByTag->second;
	}

	return nullptr;
}

void SceneManager::Delete(const std::string& name)
{
	auto sceneByName = m_ScenesByName.find(name);
	if (sceneByName != m_ScenesByName.end())
	{
		auto sceneByTag = m_ScenesByTag.find(sceneByName->second->GetTag());
		if (sceneByTag != m_ScenesByTag.end())
		{
			m_ScenesByTag.erase(sceneByTag);
		}

		sceneByName->second->Clear();
		m_ScenesByName.erase(sceneByName);
	}
}

void SceneManager::ShutDown()
{
	for (auto sceneByName : m_ScenesByName)
	{
		sceneByName.second->Clear();
	}

	m_ScenesByName.clear();

	for (auto sceneByTag : m_ScenesByTag)
	{
		sceneByTag.second->Clear();
	}

	m_ScenesByTag.clear();
}
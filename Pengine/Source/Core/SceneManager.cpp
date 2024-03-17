#include "SceneManager.h"

#include "MaterialManager.h"

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
	if (const auto sceneByName = m_ScenesByName.find(name);
		sceneByName != m_ScenesByName.end())
	{
		return sceneByName->second;
	}

	return nullptr;
}

std::shared_ptr<Scene> SceneManager::GetSceneByTag(const std::string& tag) const
{
	if (const auto sceneByTag = m_ScenesByTag.find(tag);
		sceneByTag != m_ScenesByTag.end())
	{
		return sceneByTag->second;
	}

	return nullptr;
}

void SceneManager::Delete(const std::string& name)
{
	if (const auto sceneByName = m_ScenesByName.find(name);
		sceneByName != m_ScenesByName.end())
	{
		if (const auto sceneByTag = m_ScenesByTag.find(sceneByName->second->GetTag());
			sceneByTag != m_ScenesByTag.end())
		{
			m_ScenesByTag.erase(sceneByTag);
		}

		sceneByName->second->Clear();
		m_ScenesByName.erase(sceneByName);
	}
}

void SceneManager::ShutDown()
{
	for (const auto& [name, scene] : m_ScenesByName)
	{
		scene->Clear();
	}

	m_ScenesByName.clear();

	for (const auto& [tag, scene] : m_ScenesByTag)
	{
		scene->Clear();
	}

	m_ScenesByTag.clear();
}
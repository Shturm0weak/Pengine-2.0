#pragma once

#include "Core.h"
#include "Scene.h"

namespace Pengine
{

	class PENGINE_API SceneManager
	{
	public:
		static SceneManager& GetInstance();

		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;

		std::shared_ptr<Scene> Create(const std::string& name, const std::string& tag);

		std::shared_ptr<Scene> GetSceneByName(const std::string& name) const;

		std::shared_ptr<Scene> GetSceneByTag(const std::string& tag) const;

		size_t GetScenesCount() const { return m_ScenesByName.size(); }

		const std::unordered_map<std::string, std::shared_ptr<Scene>>& GetScenes() const { return m_ScenesByName; }

		void Delete(const std::string& name);

		void Delete(std::shared_ptr<Scene>& scene);

		void ShutDown();

	private:
		SceneManager() = default;
		~SceneManager() = default;

		std::unordered_map<std::string, std::shared_ptr<Scene>> m_ScenesByName;
		std::unordered_map<std::string, std::shared_ptr<Scene>> m_ScenesByTag;
	};

}
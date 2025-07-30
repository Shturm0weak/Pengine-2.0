#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API ComponentSystem
	{
	public:
		virtual ~ComponentSystem() = default;

		virtual void OnUpdate(const float deltaTime, std::shared_ptr<class Scene> scene) {}

		virtual void OnPrePhysicsUpdate(const float deltaTime, std::shared_ptr<class Scene> scene) {}

		virtual void OnPostPhysicsUpdate(const float deltaTime, std::shared_ptr<class Scene> scene) {}

		virtual std::map<std::string, std::function<void(std::shared_ptr<class Entity>)>> GetRemoveCallbacks() { return {}; }
	};

}

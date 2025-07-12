#pragma once

#include "../Core/Core.h"

#include "ComponentSystem.h"

namespace Pengine
{

	class PENGINE_API EntityAnimatorSystem : public ComponentSystem
	{
	public:
		virtual ~EntityAnimatorSystem() override = default;

		virtual void OnUpdate(const float deltaTime, std::shared_ptr<class Scene> scene);
	};

}

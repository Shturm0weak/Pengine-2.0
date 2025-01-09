#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API ComponentSystem
	{
	public:
		virtual ~ComponentSystem() = default;

		virtual void OnUpdate(const float deltaTime, std::shared_ptr<class Scene> scene) {}
	};

}

#pragma once

#include "../Core/Core.h"

#include "ComponentSystem.h"

namespace Pengine
{

	class PENGINE_API UISystem : public ComponentSystem
	{
	public:
		virtual ~UISystem() override = default;

		virtual void OnUpdate(const float deltaTime, std::shared_ptr<class Scene> scene) override;
	};

}

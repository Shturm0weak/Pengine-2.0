#pragma once

#include "../Core/Core.h"
#include "../Core/Component.h"

namespace Pengine
{

	class PENGINE_API PointLight : public Component
	{
	public:
		glm::vec3 color = { 1.0f, 1.0f, 1.0f };

		float constant = 1.0f;
		float linear = 0.09f;
		float quadratic = 0.032f;

		static Component* Create(GameObject* owner);

	protected:

		virtual void Copy(const Component& component) override;

		virtual Component* New(GameObject* owner) override;

		virtual void Delete() override;
	private:
	};

}
#pragma once

#include "../Core/Core.h"
#include "../Core/Component.h"

namespace Pengine
{

	class PENGINE_API Renderer3D : public Component
	{
	public:
		std::shared_ptr<class Mesh> mesh;
		std::shared_ptr<class Material> material;

		static Component* Create(GameObject* owner);
	protected:

		virtual void Copy(const Component& component) override;

		virtual Component* New(GameObject* owner) override;
	private:
	};

}
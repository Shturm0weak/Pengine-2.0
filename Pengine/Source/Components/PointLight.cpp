#include "PointLight.h"

#include "../Core/Scene.h"

using namespace Pengine;

Component* PointLight::Create(GameObject* owner)
{
	PointLight* pointLight = new PointLight();
	owner->GetScene()->m_PointLights.emplace_back(pointLight);
	pointLight->m_Owner = owner;

	return pointLight;
}

void PointLight::Copy(const Component& component)
{
	PointLight& pointLight = *(PointLight*)&component;
	m_Type = component.GetType();

	color = pointLight.color;
	constant = pointLight.constant;
	linear = pointLight.linear;
	quadratic = pointLight.quadratic;
}

Component* PointLight::New(GameObject* owner)
{
	return Create(owner);
}

void PointLight::Delete()
{
	Utils::Erase(m_Owner->GetScene()->m_PointLights, this);
	delete this;
}

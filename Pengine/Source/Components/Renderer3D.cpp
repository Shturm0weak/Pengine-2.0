#include "Renderer3D.h"

#include "../Graphics/Mesh.h"
#include "../Graphics/Material.h"

using namespace Pengine;

Component* Renderer3D::Create(GameObject* owner)
{
	return new Renderer3D();
}

void Renderer3D::Copy(const Component& component)
{
	Renderer3D& r3d = *(Renderer3D*)&component;
	SetType(r3d.GetType());
	mesh = r3d.mesh;
	material = r3d.material;
}

Component* Renderer3D::New(GameObject* owner)
{
	return Create(owner);
}
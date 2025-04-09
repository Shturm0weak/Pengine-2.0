#include "Renderer3D.h"

#include "../Core/MaterialManager.h"
#include "../Core/MeshManager.h"

using namespace Pengine;

Renderer3D::~Renderer3D()
{
	MaterialManager::GetInstance().DeleteMaterial(material);
	MeshManager::GetInstance().DeleteMesh(mesh);
}

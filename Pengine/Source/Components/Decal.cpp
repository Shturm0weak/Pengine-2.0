#include "Decal.h"

#include "../Core/MaterialManager.h"

using namespace Pengine;

Decal::~Decal()
{
	MaterialManager::GetInstance().DeleteMaterial(material);
}

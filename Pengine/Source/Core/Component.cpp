#include "Component.h"

using namespace Pengine;

Component* Component::CreateCopy(GameObject* newOwner)
{
	Component* component = New(newOwner);
	*component = *this;

	return component;
}

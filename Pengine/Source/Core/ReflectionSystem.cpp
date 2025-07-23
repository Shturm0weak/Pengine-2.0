#include "ReflectionSystem.h"

using namespace Pengine;

ReflectionSystem& ReflectionSystem::GetInstance()
{
	static ReflectionSystem reflectionSystem;
	return reflectionSystem;
}

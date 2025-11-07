#pragma once

#include "Core/ReflectionSystem.h"

struct Enemy
{
	PROPERTY(float, health, 100.0f);
};
REGISTER_CLASS(Enemy);

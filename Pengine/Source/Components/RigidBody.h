#pragma once

#include "../Core/Core.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace Pengine
{

	class PENGINE_API RigidBody
	{
	public:

		enum class Type
		{
			Box,
			Sphere
		};

		struct Box
		{
			glm::vec3 halfExtents = { 1.0f, 1.0f, 1.0f };
		};

		struct Sphere
		{
			float radius = 1.0f;
		};

		union Shape
		{
			Box box;
			Sphere sphere;
		};
		
		Shape shape = Shape(Box());
		Type type = Type::Box;
		JPH::BodyID id;

		float mass = 1.0f;

		bool isStatic = false;
		bool isValid = false;
	};

}

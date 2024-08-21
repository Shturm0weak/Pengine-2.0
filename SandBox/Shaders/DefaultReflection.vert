#version 450

layout(location = 0) in vec3 positionA;

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

void main()
{
	gl_Position = vec4(positionA, 1.0f);
}

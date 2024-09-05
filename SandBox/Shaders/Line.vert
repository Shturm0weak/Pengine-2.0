#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec3 colorA;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 color;

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

void main()
{
	gl_Position = camera.viewProjectionMat4 * vec4(positionA, 1.0);
	worldPosition = positionA;
	color = colorA;
}

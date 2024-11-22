#version 450

layout(location = 0) in vec3 positionA;

layout(location = 0) out vec3 uv;

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

void main()
{
	gl_Position = camera.projectionMat4 * camera.inverseRotationMat4 * vec4(positionA, 1.0f);
	gl_Position.z = gl_Position.w;
	uv = positionA;
}

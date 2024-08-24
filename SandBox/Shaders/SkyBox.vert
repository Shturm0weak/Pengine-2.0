#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec3 normalA;
layout(location = 3) in vec3 tangentA;
layout(location = 4) in vec3 bitangentA;

layout(location = 0) out vec3 uv;

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

void main()
{
	gl_Position = camera.projectionMat4 * inverse(camera.rotationMat4) * vec4(positionA, 1.0);
	uv = positionA;
}

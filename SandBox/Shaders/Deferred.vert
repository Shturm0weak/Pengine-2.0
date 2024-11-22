#version 450

layout(location = 0) in vec2 positionA;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec2 viewRay;

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

void main()
{
	gl_Position = vec4(positionA.x, positionA.y, 0.0f, 1.0f);
	uv = (positionA + vec2(1.0f)) * vec2(0.5f);
	uv.y = 1.0f - uv.y;
	viewRay.x = -positionA.x * camera.aspectRatio * camera.tanHalfFOV;
	viewRay.y = -positionA.y * camera.tanHalfFOV;
}

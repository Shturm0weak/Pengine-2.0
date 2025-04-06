#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec3 normalA;
layout(location = 3) in vec3 tangentA;
layout(location = 4) in vec3 bitangentA;
layout(location = 5) in uint colorA;
layout(location = 6) in mat4 transformA;
layout(location = 10) in mat3 inverseTransformA;

layout(location = 0) out vec3 normalViewSpace;
layout(location = 1) out vec3 tangentViewSpace;
layout(location = 2) out vec3 bitangentViewSpace;
layout(location = 3) out vec2 uv;
layout(location = 4) out vec4 color;

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

#include "Shaders/Includes/DefaultMaterial.h"

layout(set = 1, binding = 6) uniform GBufferMaterial
{
	DefaultMaterial material;
};

void main()
{
	gl_Position = camera.viewProjectionMat4 * transformA * vec4(positionA, 1.0f);

	normalViewSpace = normalize(mat3(camera.viewMat4) * inverseTransformA * normalize(normalA));
	tangentViewSpace = normalize(mat3(camera.viewMat4) * inverseTransformA * normalize(tangentA));
	bitangentViewSpace = normalize(mat3(camera.viewMat4) * inverseTransformA * normalize(bitangentA));

	uv = uvA * material.uvTransform.xy + material.uvTransform.zw;

	color = unpackUnorm4x8(colorA);
	//color = vec4(
	//	float(colorA & uint(0xFF000000) >> 24),
	//	float(colorA & uint(0x00FF0000) >> 16),
	//	float(colorA & uint(0x0000FF00) >> 8),
	//	float(colorA & uint(0x000000FF) >> 0)) * 0.003921568627451f;
}

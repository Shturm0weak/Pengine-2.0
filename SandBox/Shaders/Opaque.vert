#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec3 normalA;
layout(location = 3) in vec3 tangentA;
layout(location = 4) in vec3 bitangentA;
layout(location = 5) in mat4 transformA;
layout(location = 9) in mat3 inverseTransformA;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec3 worldPosition;
layout(location = 2) out vec2 uv;
layout(location = 3) out mat3 TBN;

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

void main()
{
	worldPosition = (transformA * vec4(positionA, 1.0)).xyz;
	gl_Position = camera.viewProjectionMat4 * vec4(worldPosition, 1.0);
	normal = normalize(inverseTransformA * normalA);
	vec3 tangent = normalize(inverseTransformA * tangentA);
	vec3 bitangent = normalize(inverseTransformA * bitangentA);
	TBN = mat3(tangent, bitangent, normal);
	uv = uvA;
}

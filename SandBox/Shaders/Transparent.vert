#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec3 normalA;
layout(location = 3) in vec3 tangentA;
layout(location = 4) in vec3 bitangentA;
layout(location = 5) in mat4 transformA;
layout(location = 9) in mat3 inverseTransformA;

layout(location = 0) out vec3 viewSpaceNormal;
layout(location = 1) out vec3 viewSpacePosition;
layout(location = 2) out vec2 uv;
layout(location = 3) out mat3 TBN;

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
	viewSpacePosition = (camera.viewMat4 * transformA * vec4(positionA, 1.0f)).xyz;
	gl_Position = camera.projectionMat4 * vec4(viewSpacePosition, 1.0f);
	viewSpaceNormal = normalize(mat3(camera.viewMat4) * inverseTransformA * normalA);
	vec3 tangent = normalize(inverseTransformA * tangentA);
	vec3 bitangent = normalize(inverseTransformA * bitangentA);
	TBN = mat3(tangent, bitangent, viewSpaceNormal);
	uv = uvA * material.uvTransform.xy + material.uvTransform.zw;
}

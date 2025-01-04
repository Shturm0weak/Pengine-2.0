#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec3 normalA;
layout(location = 3) in vec3 tangentA;
layout(location = 4) in vec3 bitangentA;
layout(location = 5) in mat4 transformA;
layout(location = 9) in mat3 inverseTransformA;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec3 tangent;
layout(location = 2) out vec3 bitangent;
layout(location = 3) out vec2 uv;


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
	normal = normalize(mat3(camera.viewMat4) * inverseTransformA * normalA);
	tangent = normalize(inverseTransformA * tangentA);
	bitangent = normalize(inverseTransformA * bitangentA);
	uv = uvA * material.uvTransform.xy + material.uvTransform.zw;
}

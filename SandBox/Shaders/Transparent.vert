#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec3 normalA;
layout(location = 3) in vec4 tangentA;
layout(location = 4) in uint colorA;
layout(location = 5) in mat4 transformA;
layout(location = 9) in mat3 inverseTransformA;

layout(location = 0) out vec3 positionViewSpace;
layout(location = 1) out vec3 positionWorldSpace;
layout(location = 2) out vec3 normalViewSpace;
layout(location = 3) out vec3 tangentViewSpace;
layout(location = 4) out vec3 bitangentViewSpace;
layout(location = 5) out vec2 uv;
layout(location = 6) out vec4 color;

#include "Shaders/Includes/Camera.h"
layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

#include "Shaders/Includes/DefaultMaterial.h"
layout(set = 1, binding = 0) uniform GBufferMaterial
{
	DefaultMaterial material;
};

void main()
{
	positionWorldSpace = (transformA * vec4(positionA, 1.0f)).xyz;
	positionViewSpace = (camera.viewMat4 * vec4(positionWorldSpace, 1.0f)).xyz;
	gl_Position = camera.projectionMat4 * vec4(positionViewSpace, 1.0f);

	vec3 normal = normalize(normalA);
	vec3 tangent = normalize(tangentA.xyz);
	vec3 bitangent = normalize(cross(normal, tangent) * tangentA.w);

	mat3 viewMat3 = mat3(camera.viewMat4) * inverseTransformA;

	normalViewSpace = normalize(viewMat3 * normal);
	tangentViewSpace = normalize(viewMat3 * tangent);
	bitangentViewSpace = normalize(viewMat3 * bitangent);

	uv = uvA * material.uvTransform.xy + material.uvTransform.zw;
	
	color = unpackUnorm4x8(colorA);
}

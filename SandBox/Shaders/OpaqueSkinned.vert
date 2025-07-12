#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec3 normalA;
layout(location = 3) in vec4 tangentA;
layout(location = 4) in uint colorA;
layout(location = 5) in vec4 weightsA;
layout(location = 6) in ivec4 boneIdsA;
layout(location = 7) in mat4 transformA;
layout(location = 11) in mat3 inverseTransformA;

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
layout(set = 1, binding = 0) uniform GBufferMaterial
{
	DefaultMaterial material;
};

#include "Shaders/Includes/Bones.h"
layout(set = 2, binding = 0) uniform BoneMatrices
{
	mat4 boneMatrices[MAX_BONES];
};

void main()
{
	vec3 normal = normalize(normalA);
	vec3 tangent = normalize(tangentA.xyz);
	vec3 bitangent = normalize(cross(normal, tangent) * tangentA.w);

	vec4 totalPosition = vec4(0.0f);
	vec3 totalNormal = vec3(0.0f);
	vec3 totalTangent = vec3(0.0f);
	vec3 totalBitangent = vec3(0.0f);
	for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
	{
		if(boneIdsA[i] == -1)
			continue;
		if(boneIdsA[i] >= MAX_BONES)
		{
			totalPosition = vec4(positionA, 1.0f);
			totalNormal = normal;
			totalTangent = tangent;
			totalBitangent = bitangent;
			break;
		}
		vec4 localPosition = boneMatrices[boneIdsA[i]] * vec4(positionA, 1.0f);
		totalPosition += localPosition * weightsA[i];

		mat3 boneMat3 = mat3(boneMatrices[boneIdsA[i]]);

		vec3 localNormal = boneMat3 * normal;
		totalNormal += localNormal * weightsA[i];

		vec3 localTangent = boneMat3 * tangent;
		totalTangent += localTangent * weightsA[i];

		vec3 localBitangent = boneMat3 * bitangent;
		totalBitangent += localBitangent * weightsA[i];
	}

	gl_Position = camera.viewProjectionMat4 * transformA * totalPosition;

	mat3 viewMat3 = mat3(camera.viewMat4) * inverseTransformA;

	normalViewSpace = normalize(viewMat3 * normalize(totalNormal));
	tangentViewSpace = normalize(viewMat3 * normalize(totalTangent));
	bitangentViewSpace = normalize(viewMat3 * normalize(totalBitangent));

	uv = uvA * material.uvTransform.xy + material.uvTransform.zw;

	color = unpackUnorm4x8(colorA);
}

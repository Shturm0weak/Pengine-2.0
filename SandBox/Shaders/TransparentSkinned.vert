#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec3 normalA;
layout(location = 3) in vec3 tangentA;
layout(location = 4) in vec3 bitangentA;
layout(location = 5) in uint colorA;
layout(location = 6) in vec4 weightsA;
layout(location = 7) in ivec4 boneIdsA;
layout(location = 8) in mat4 transformA;
layout(location = 12) in mat3 inverseTransformA;

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

#include "Shaders/Includes/Bones.h"
layout(set = 4, binding = 0) uniform BonesMatrices
{
	mat4 bonesMatrices[MAX_BONES];
};

void main()
{
	vec4 totalPosition = vec4(0.0f);
	vec3 totalNormal = vec3(0.0f);
	vec3 totalTangent = vec3(0.0f);
	vec3 totalBitangent = vec3(0.0f);
	for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
	{
		if(boneIdsA[i] == -1)
			continue;
		if(boneIdsA[i] >= MAX_BONES)
		{
			totalPosition = vec4(positionA, 1.0f);
			totalNormal = normalA;
			totalTangent = tangentA;
			totalBitangent = bitangentA;
			break;
		}
		vec4 localPosition = bonesMatrices[boneIdsA[i]] * vec4(positionA,1.0f);
		totalPosition += localPosition * weightsA[i];

		vec3 localNormal = mat3(bonesMatrices[boneIdsA[i]]) * normalA;
		totalNormal += localNormal * weightsA[i];

		vec3 localTangent = mat3(bonesMatrices[boneIdsA[i]]) * tangentA;
		totalTangent += localTangent * weightsA[i];

		vec3 localBitangent = mat3(bonesMatrices[boneIdsA[i]]) * bitangentA;
		totalBitangent += localBitangent * weightsA[i];
	}

	positionWorldSpace = (transformA * totalPosition).xyz;
	positionViewSpace = (camera.viewMat4 * vec4(positionWorldSpace, 1.0f)).xyz;
	gl_Position = camera.projectionMat4 * vec4(positionViewSpace, 1.0f);

	normalViewSpace = normalize(mat3(camera.viewMat4) * inverseTransformA * normalize(totalNormal));
	tangentViewSpace = normalize(mat3(camera.viewMat4) * inverseTransformA * normalize(totalTangent));
	bitangentViewSpace = normalize(mat3(camera.viewMat4) * inverseTransformA * normalize(totalBitangent));

	uv = uvA * material.uvTransform.xy + material.uvTransform.zw;

	color = unpackUnorm4x8(colorA);
}

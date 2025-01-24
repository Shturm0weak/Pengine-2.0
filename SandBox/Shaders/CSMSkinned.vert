#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec4 weightsA;
layout(location = 3) in ivec4 boneIdsA;
layout(location = 4) in mat4 transformA;

layout(location = 0) out vec2 uv;

#include "Shaders/Includes/Bones.h"
layout(set = 2, binding = 0) uniform BonesMatrices
{
	mat4 bonesMatrices[MAX_BONES];
};

void main()
{
	vec4 totalPosition = vec4(0.0f);
	for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
	{
		if(boneIdsA[i] == -1)
		{
			continue;
		}
		if(boneIdsA[i] >= MAX_BONES)
		{
			totalPosition = vec4(positionA, 1.0f);
			break;
		}
		vec4 localPosition = bonesMatrices[boneIdsA[i]] * vec4(positionA,1.0f);
		totalPosition += localPosition * weightsA[i];
	}

	gl_Position = transformA * totalPosition;
	uv = uvA;
}

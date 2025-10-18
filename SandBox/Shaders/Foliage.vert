#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec3 normalA;
layout(location = 3) in vec4 tangentA;
layout(location = 4) in uint colorA;
layout(location = 5) in mat4 transformA;
layout(location = 9) in mat3 inverseTransformA;

layout(location = 0) out vec3 normalViewSpace;
layout(location = 1) out vec3 tangentViewSpace;
layout(location = 2) out vec3 bitangentViewSpace;
layout(location = 3) out vec2 uv;
layout(location = 4) out vec4 color;
layout(location = 5) out vec3 positionTangentSpace;
layout(location = 6) out vec3 cameraPositionTangentSpace;

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
	vec4 windParams = unpackUnorm4x8(colorA);
	float stiffness = windParams.r;
    float oscillation = windParams.g;

	float windWave = sin(camera.time * camera.wind.frequency + float(gl_VertexIndex) * oscillation);

	float windInfluence = (1.0f - stiffness) * camera.wind.strength;
	vec3 windDisplacement = camera.wind.direction * windWave * windInfluence;

	vec3 positionWorldSpace = vec3(transformA * vec4(windDisplacement + positionA, 1.0f));
	gl_Position = camera.viewProjectionMat4 * vec4(positionWorldSpace, 1.0f);

	vec3 normal = normalize(inverseTransformA * normalize(normalA));
	vec3 tangent = normalize(inverseTransformA * normalize(tangentA.xyz));
	vec3 bitangent = normalize(cross(normal, tangent) * tangentA.w);

	if (material.useParallaxOcclusion > 0)
	{
		vec3 T   = tangent;
    	vec3 B   = bitangent;
   		vec3 N   = normal;
    	mat3 TBN = transpose(mat3(T, B, N));

		cameraPositionTangentSpace = TBN * camera.position;
    	positionTangentSpace = TBN * positionWorldSpace.xyz;
	}

	normalViewSpace = normalize(mat3(camera.viewMat4) * normal);
	tangentViewSpace = normalize(mat3(camera.viewMat4) * tangent);
	bitangentViewSpace = normalize(mat3(camera.viewMat4) * bitangent);

	uv = uvA * material.uvTransform.xy + material.uvTransform.zw;

	color = vec4(1.0f);
}

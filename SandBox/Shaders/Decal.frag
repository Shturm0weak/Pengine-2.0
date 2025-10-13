#version 450

layout(location = 0) flat in mat4 inverseTransform;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outShading;
layout(location = 3) out vec4 outEmissive;

layout(set = 1, binding = 1) uniform sampler2D albedoTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessTexture;
layout(set = 1, binding = 4) uniform sampler2D aoTexture;
layout(set = 1, binding = 5) uniform sampler2D emissiveTexture;

#include "Shaders/Includes/DefaultMaterial.h"
layout(set = 1, binding = 0) uniform GBufferMaterial
{
	DefaultMaterial material;
};

#include "Shaders/Includes/Camera.h"
layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

layout(set = 2, binding = 0) uniform sampler2D depthTexture;

void main()
{
    ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
    vec2 screenUV = (vec2(pixelCoord) + vec2(0.5f)) / camera.viewportSize;
	vec2 screenPosition = screenUV * 2.0f - 1.0f;
	
	vec2 viewRay;
	viewRay.x = -screenPosition.x * camera.aspectRatio * camera.tanHalfFOV;
    viewRay.y = screenPosition.y * camera.tanHalfFOV;

    vec3 positionViewSpace = CalculatePositionFromDepth(
        texture(depthTexture, screenUV).x,
        camera.projectionMat4,
        viewRay);

    vec3 worldSpacePosition = (camera.inverseViewMat4 * vec4(positionViewSpace, 1.0f)).xyz;
    vec3 localPosition = vec4(inverseTransform * vec4(worldSpacePosition, 1.0f)).xyz;
    vec2 decalUV = localPosition.xz * 0.5f + 0.5f;

    if (abs(localPosition.x) > 1.0f || abs(localPosition.z) > 1.0f ||
        abs(localPosition.y) > 1.0f)
    {
		discard;
	}

	vec4 albedoColor = texture(albedoTexture, decalUV.xy) * material.albedoColor;
	if (material.useAlphaCutoff > 0)
	{
		if (albedoColor.a < material.alphaCutoff)
		{
			discard;
		}
	}

	vec3 metallicRoughness = texture(metallicRoughnessTexture, decalUV.xy).xyz;
	float metallic = metallicRoughness.b;
	float roughness = metallicRoughness.g;
	float ao = texture(aoTexture, decalUV.xy).r;

	outAlbedo = albedoColor;
	outShading = vec4(
		metallic * material.metallicFactor,
		roughness * material.roughnessFactor,
		ao * material.aoFactor,
		1.0f);
	outEmissive = texture(emissiveTexture, decalUV.xy) * material.emissiveColor * material.emissiveFactor;

	vec3 ddxWp = dFdx(worldSpacePosition);
	vec3 ddyWp = dFdy(worldSpacePosition);
	vec3 normal = normalize(cross(ddyWp, ddxWp));

	mat3 viewMat3 = mat3(camera.viewMat4) * transpose(mat3(inverseTransform));
	vec3 normalViewSpace = normalize(viewMat3 * normal);

	if (material.useNormalMap > 0)
	{
		vec3 bitangent = normalize(ddxWp);
		vec3 tangent = normalize(ddyWp);

		vec3 tangentViewSpace = normalize(viewMat3 * tangent);
		vec3 bitangentViewSpace = normalize(viewMat3 * bitangent);

		mat3 TBN = mat3(tangentViewSpace, bitangentViewSpace, normalViewSpace);
		vec3 normalMap = texture(normalTexture, decalUV.xy).xyz;
		normalMap = normalMap * 2.0f - 1.0f;
		outNormal = vec4(normalize(TBN * normalMap), 1.0f);
	}
	else
	{
		outNormal = vec4(normalViewSpace, 1.0f);
	}
}

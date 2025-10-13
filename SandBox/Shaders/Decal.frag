#version 450

layout(location = 0) flat in mat4 inverseTransform;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outShading;
layout(location = 2) out vec4 outEmissive;

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

layout(set = 2, binding = 0) uniform sampler2D depthGBufferTexture;
layout(set = 2, binding = 1, rgba16f) uniform image2D normalGBufferTexture;

void main()
{
    ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
    vec2 screenUV = (vec2(pixelCoord) + vec2(0.5f)) / camera.viewportSize;
	vec2 screenPosition = screenUV * 2.0f - 1.0f;
	
	vec2 viewRay;
	viewRay.x = -screenPosition.x * camera.aspectRatio * camera.tanHalfFOV;
    viewRay.y = screenPosition.y * camera.tanHalfFOV;

    vec3 positionViewSpace = CalculatePositionFromDepth(
        texture(depthGBufferTexture, screenUV).x,
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

	vec4 gbufferNormal = imageLoad(normalGBufferTexture, pixelCoord);
	vec3 normal = gbufferNormal.xyz;

	vec3 up = normalize(mat3(camera.viewMat4) * vec3(0.0f, 1.0f, 0.0f));
	if (abs(dot(normal, up)) > 0.999f)
	{
		up = normalize(mat3(camera.viewMat4) * vec3(1.0f, 0.0f, 0.0f));
	}

	vec3 tangent = normalize(cross(normal, up));
	vec3 bitangent = cross(normal, tangent);

	if (material.useNormalMap > 0)
	{
		mat3 TBN = mat3(tangent, bitangent, normal);
		vec3 normalMap = texture(normalTexture, decalUV.xy).xyz;
		normalMap = normalMap * 2.0f - 1.0f;
		imageStore(normalGBufferTexture, pixelCoord, vec4(normalize(TBN * normalMap), gbufferNormal.a));
	}
}

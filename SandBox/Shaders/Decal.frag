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
layout(set = 1, binding = 6) uniform sampler2D heightTexture;

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
layout(set = 2, binding = 1, rg16f) uniform image2D normalGBufferTexture;

#include "Shaders/Includes/ParallaxOcclusionMapping.h"

mat3 ConstructTBN( vec3 N, vec3 p, vec2 uv )
{
	// get edge vectors of the pixel triangle
	vec3 dp1 = dFdx( p );
	vec3 dp2 = dFdy( p );
	vec2 duv1 = dFdx( uv );
	vec2 duv2 = dFdy( uv );
	// solve the linear system
	vec3 dp2perp = cross( dp2, N );
	vec3 dp1perp = cross( N, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
	// construct a scale-invariant frame
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
	return mat3( T * invmax, B * invmax, N ); }

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

    vec3 positionWorldSpace = (camera.inverseViewMat4 * vec4(positionViewSpace, 1.0f)).xyz;
    vec3 localPosition = vec4(inverseTransform * vec4(positionWorldSpace, 1.0f)).xyz;
    vec2 decalUV = localPosition.xz * 0.5f + 0.5f;

    if (abs(localPosition.x) > 1.0f || abs(localPosition.z) > 1.0f ||
        abs(localPosition.y) > 1.0f)
    {
		discard;
	}

	vec3 gbufferNormal = OctDecode(imageLoad(normalGBufferTexture, pixelCoord).xy);
	
	vec2 uvForTBN = decalUV;
	if (material.useParallaxOcclusion > 0)
	{
		vec3 normalWorldSpace = normalize(mat3(camera.inverseViewMat4) * normalize(gbufferNormal));
		mat3 TBN = transpose(ConstructTBN(normalWorldSpace, positionWorldSpace, uvForTBN));

		vec3 cameraPositionTangentSpace = TBN * camera.position;
    	vec3 positionTangentSpace = TBN * positionWorldSpace;
		vec3 viewDirection = normalize(cameraPositionTangentSpace - positionTangentSpace);
		decalUV = ParallaxOcclusionMapping(
			heightTexture,
			decalUV,
			viewDirection,
			material.minParallaxLayers,
			material.maxParallaxLayers,
			-material.parallaxHeightScale);
	}

	vec4 albedoColor = texture(albedoTexture, decalUV) * material.albedoColor;
	if (material.useAlphaCutoff > 0)
	{
		if (albedoColor.a < material.alphaCutoff)
		{
			discard;
		}
	}

	vec3 metallicRoughness = texture(metallicRoughnessTexture, decalUV).xyz;
	float metallic = metallicRoughness.b;
	float roughness = metallicRoughness.g;
	float ao = texture(aoTexture, decalUV).r;

	outAlbedo = albedoColor;
	outShading = vec4(
		metallic * material.metallicFactor,
		roughness * material.roughnessFactor,
		ao * material.aoFactor,
		1.0f);
	outEmissive = texture(emissiveTexture, decalUV) * material.emissiveColor * material.emissiveFactor;
	
	if (material.useNormalMap > 0)
	{
		mat3 TBN = ConstructTBN(gbufferNormal, positionViewSpace, uvForTBN);
		vec3 normalMap = texture(normalTexture, decalUV).xyz;
		normalMap = normalMap * 2.0f - 1.0f;
		imageStore(normalGBufferTexture, pixelCoord, vec4(OctEncode(normalize(TBN * normalMap)), 0.0f, 0.0f));
	}
	else
	{
		imageStore(normalGBufferTexture, pixelCoord, vec4(OctEncode(gbufferNormal), 0.0f, 0.0f));
	}
}

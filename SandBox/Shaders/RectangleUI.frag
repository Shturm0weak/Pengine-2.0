#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 positionInPixels;
layout(location = 1) in vec2 uv;
layout(location = 2) flat in float cornerRadius;
layout(location = 3) flat in int instanceIndex;

layout(set = 0, binding = 0) uniform sampler2D imageTexture;

struct Instance
{
	vec4 color;
	vec4 quadCenterAndHalfSize;

	/**
     * { isText, empty, empty. empty }
     */
	ivec4 customData;
};

// Also need to change in Core/UIRenderer.cpp.
#define MAX_BATCH_QUAD_COUNT 10000
layout(set = 1, binding = 0) buffer readonly InstanceBuffer
{
	Instance instances[MAX_BATCH_QUAD_COUNT];
};

float RoundedQuadSDF(in vec2 position, in vec2 quadCenter, in vec2 quadHalfSize)
{
	vec2 distance = abs(quadCenter - position) - quadHalfSize + vec2(cornerRadius, cornerRadius);
	return min(max(distance.x, distance.y), 0.0f) + length(max(distance, 0.0f)) - cornerRadius;
}

void main()
{
	Instance instance = instances[instanceIndex];

	float roundFactor = RoundedQuadSDF(positionInPixels, instance.quadCenterAndHalfSize.xy, instance.quadCenterAndHalfSize.zw);
	roundFactor = smoothstep(0.0f, -1.0f, roundFactor);

	vec4 finalColor;
	if (instance.customData.x == 1)
	{
		// For now it is supposed to be only 1 channel texture atlas.
		finalColor = texture(imageTexture, uv).rrrr * instance.color;
	}
	else
	{
		finalColor = texture(imageTexture, uv) * instance.color;
	}

	if (roundFactor < 0.01f)
	{
		discard;
	}
	else
	{
		outColor = vec4(finalColor.xyz, finalColor.a);
	}
}

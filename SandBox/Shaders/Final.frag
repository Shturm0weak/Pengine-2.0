#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D deferredTexture;
layout(set = 0, binding = 1) uniform sampler2D bloomTexture;

layout(set = 0, binding = 2) uniform PostProcessBuffer
{
	int toneMapperIndex;
	float gamma;
};

vec3 aces(vec3 x)
{
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	vec3 bloom = texture(bloomTexture, uv).xyz;
	vec3 deferred = texture(deferredTexture, uv).xyz;

	if (toneMapperIndex == 0)
	{
		outColor = vec4(pow(deferred + bloom, vec3(1.0f / gamma)), 1.0f);
	}
	else if (toneMapperIndex == 1)
	{
		outColor = vec4(pow(aces(deferred + bloom), vec3(1.0f / gamma)), 1.0f);
	}
}

#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D albedoTexture;
layout(set = 0, binding = 1) uniform sampler2D normalTexture; 
layout(set = 0, binding = 2) uniform sampler2D positionTexture; 

layout(set = 0, binding = 3) uniform Light
{
	vec3 color;
	float constant;

	float linear;
	float quadratic;
	float use;
	float __unused1;

	vec3 lightPosition;
	float __unused2;
};

void main()
{
	vec3 albedoColor = texture(albedoTexture, uv).xyz;
	vec3 normal = texture(normalTexture, uv).xyz;
	vec3 position = texture(positionTexture, uv).xyz;

	if(use > 0.5f)
	{
		vec3 ambient = color * vec3(0.3f);
		vec3 lightDir = normalize(lightPosition - position);
		float diff = max(dot(normal, lightDir), 0.0f);
		vec3 diffuse = color * diff;

		float distance    = length(lightPosition - position);
		float attenuation = 1.0f / (constant + linear * distance + quadratic * (distance * distance));

		diffuse   *= attenuation;

		vec3 result = (ambient + diffuse) * albedoColor;
		outColor = vec4(result, 1.0f);
	}
	else
	{
		outColor = vec4(0.0f);
	}
}

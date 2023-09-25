#version 450

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 worldPosition;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPosition;

layout(set = 1, binding = 0) uniform sampler2D albedo;

struct Material
{
	vec4 color;
	vec4 ambient;
};

layout(set = 1, binding = 1) uniform MaterialsBuffer
{
    Material material;
};

void main()
{
	// For more depth image. Remove after test
	vec3 lightPosition = vec3(0.0, 3.0, 0.0);
	vec3 lightDirection = normalize(lightPosition - worldPosition);
	float diffuse = max(dot(normal, lightDirection), 0.0);

	outAlbedo = texture(albedo, uv) * material.color * diffuse * 2.0f + texture(albedo, uv) * material.color * material.ambient.x;
	outNormal = vec4(normal, 1.0);
	outPosition = vec4(worldPosition, 1.0);
}
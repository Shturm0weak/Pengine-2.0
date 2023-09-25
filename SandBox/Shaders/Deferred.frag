#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D textures[3]; 

layout(set = 0, binding = 1) uniform TexturesBuffer
{
	int albedo;
	int normal;
	int position;
};

vec4 positionColor = texture(textures[position], uv);
vec4 normalColor = texture(textures[normal], uv);
vec4 albedoColor = texture(textures[albedo], uv);

void main()
{
	outColor = normalColor;
}
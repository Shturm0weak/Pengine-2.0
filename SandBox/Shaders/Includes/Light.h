struct PointLight
{
	vec3 color;
	float constant;
	
	vec3 position;
	float linear;
	
	float quadratic;
};

struct DirectionalLight
{
	vec3 color;
	float intensity;
	vec3 direction;
};

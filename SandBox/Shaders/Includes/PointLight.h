struct PointLight
{
	vec3 color;
	float intensity;
	
	vec3 position;
	float radius;
};

vec3 CalculatePointLight(
	in PointLight light,
	in vec3 position,
	in vec3 normal)
{
	vec3 direction = normalize(light.position - position);
	vec3 diffuse = light.color * light.intensity * max(dot(normal, direction), 0.0f);

	float distance    = length(light.position - position);
	float attenuation = max(0.0f,
		(1.0f / (distance * distance)) - (1.0f / (light.radius * light.radius)));

	diffuse *= attenuation;

	return diffuse;
}

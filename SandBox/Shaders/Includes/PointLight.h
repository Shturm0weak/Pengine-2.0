struct PointLight
{
	vec3 color;
	float constant;
	
	vec3 position;
	float linear;
	
	float quadratic;
};

vec3 CalculatePointLight(PointLight light, vec3 position, vec3 normal)
{
	vec3 direction = normalize(light.position - position);
	float diff = max(dot(normal, direction), 0.0f);
	vec3 diffuse = light.color * diff;

	float distance    = length(light.position - position);
	float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	diffuse   *= attenuation;

	return diffuse;
}

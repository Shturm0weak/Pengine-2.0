#include "Shaders/Includes/CommonPBR.h"

struct DirectionalLight
{
	vec3 color;
	float intensity;
	vec3 direction;
};

vec3 CalculateDirectionalLight(
	DirectionalLight light,
	vec3 viewDirection,
	vec3 basicReflectivity,
	vec3 normal,
	vec3 albedo,
	float metallic,
	float roughness,
	float ao)
{
	vec3 H = normalize(viewDirection + light.direction);

	vec3 radiance = light.color * light.intensity;
	vec3 ambient = 0.01 * radiance * ao;

	float NdotV = max(dot(normal, viewDirection), 0.0000001);
	float NdotL = max(dot(normal, light.direction), 0.0000001);
	float HdotV = max(dot(H, viewDirection), 0.0);
	float NdotH = max(dot(normal, H), 0.0);

	float D = DistributionGGX(NdotH, roughness);
	float G = GeometrySmith(NdotV, NdotL, roughness);
	vec3 F = FresnelSchlick(HdotV, basicReflectivity);

	vec3 specular = D * G * F;
	specular /= 4.0 * NdotV * NdotL;// + 0.0001;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;

	kD *= 1.0 - metallic;

	return ambient * albedo + (kD * albedo / PI + specular) * radiance * NdotL;
}
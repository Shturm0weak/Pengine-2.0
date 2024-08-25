struct Camera
{
	mat4 viewProjectionMat4;
	mat4 projectionMat4;
	mat4 inverseRotationMat4;
	vec3 position;
	vec3 direction;
	float time;
};

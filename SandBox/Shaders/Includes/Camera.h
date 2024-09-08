struct Camera
{
	mat4 viewProjectionMat4;
	mat4 projectionMat4;
	mat4 viewMat4;
	mat4 inverseRotationMat4;
	vec3 position;
	vec3 direction;
	vec2 viewportSize;
	float aspectRatio;
	float tanHalfFOV;
	float time;
	float zNear;
	float zFar;
};

vec3 CalculatePositionFromDepth(float depth, mat4 projectionMat4, vec2 viewRay)
{
    float z = -projectionMat4[3][2] / (depth + projectionMat4[2][2]);
    float x = viewRay.x * z;
    float y = viewRay.y * z;
    return vec3(x, y, z);
}

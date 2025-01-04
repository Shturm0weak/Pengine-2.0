struct Camera
{
	mat4 viewProjectionMat4;
	mat4 projectionMat4;
	mat4 viewMat4;
	mat4 inverseViewMat4;
	mat4 inverseRotationMat4;
	vec2 viewportSize;
	float aspectRatio;
	float tanHalfFOV;
	float time;
	float deltaTime;
	float zFar;
	float zNear;
};

vec3 CalculatePositionFromDepth(
	in float depth,
	in mat4 projectionMat4,
	in vec2 viewRay)
{
    float z = -projectionMat4[3][2] / (depth + projectionMat4[2][2]);
    float x = viewRay.x * z;
    float y = viewRay.y * z;
    return vec3(x, y, z);
}

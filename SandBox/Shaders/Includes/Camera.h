struct Wind
{
	float strength;
	vec3 direction;
    float frequency;
};

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
	vec3 position;
	Wind wind;
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

vec2 OctEncode(vec3 n)
{
	n.xy /= (abs(n.x) + abs(n.y) + abs(n.z));

	if (n.z < 0.0f)
	{
	    n.xy = (1.0f - abs(n.yx)) * sign(n.xy);
	}

	return n.xy * 0.5f + 0.5f;
}

vec3 OctDecode(vec2 f)
{
	f = f * 2.0f - 1.0f;
	vec3 n = vec3(f.x, f.y, 1.0f - abs(f.x) - abs(f.y));

	if (n.z < 0.0f)
	{
	    n.xy = (1.0f - abs(n.yx)) * sign(n.xy);
	}

	return normalize(n);
}
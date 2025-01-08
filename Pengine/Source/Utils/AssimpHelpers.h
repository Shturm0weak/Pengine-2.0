#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

namespace Pengine::Utils
{

	inline glm::mat4 AiMat4ToGlmMat4(const aiMatrix4x4& from)
	{
		glm::mat4 to;
		//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
		to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
		to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
		to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
		return to;
	};

	inline glm::vec3 AiVec3ToGlmVec3(const aiVector3D& vec)
	{
		return glm::vec3(vec.x, vec.y, vec.z);
	};

	inline glm::quat AiQuatToGlmQuat(const aiQuaternion& quat)
	{
		return glm::quat(quat.w, quat.x, quat.y, quat.z);
	};

}

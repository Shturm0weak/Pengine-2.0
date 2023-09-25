#include "MeshManager.h"

#include "Logger.h"
#include "Serializer.h"

#include "FileFormatNames.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Pengine;

MeshManager& MeshManager::GetInstance()
{
	static MeshManager meshManager;
	return meshManager;
}

std::shared_ptr<Mesh> MeshManager::LoadMesh(const std::string& filepath)
{
	if (std::shared_ptr<Mesh> mesh = GetMesh(filepath))
	{
		return mesh;
	}
	else
	{
		mesh = Serializer::DeserializeMesh(filepath);
		if (!mesh)
		{
			FATAL_ERROR(filepath + ":There is no such mesh!")
		}

		m_MeshesByFilepath.emplace(filepath, mesh);

		return mesh;
	}
}

std::shared_ptr<Mesh> MeshManager::GetMesh(const std::string& filepath)
{
	auto meshByFilepath = m_MeshesByFilepath.find(filepath);
	if (meshByFilepath != m_MeshesByFilepath.end())
	{
		return meshByFilepath->second;
	}

	return nullptr;
}

std::vector<std::shared_ptr<Mesh>> MeshManager::GenerateMeshes(const std::string& filepath)
{
	std::vector<std::shared_ptr<Mesh>> meshes = LoadIntermediate(filepath);
	for (const auto& mesh : meshes)
	{
		Serializer::SerializeMesh(Utils::ExtractDirectoryFromFilePath(filepath), mesh);

		m_MeshesByFilepath.emplace(mesh->GetFilepath(), mesh);
	}

	if (meshes.size() > 1)
	{
		std::string meshesFilepath = Utils::EraseFileFormat(filepath) + "." + FileFormats::Meshes();
		Serializer::SerializeMeshes(meshesFilepath, meshes);
	}

	return meshes;
}

std::vector<std::shared_ptr<Mesh>> MeshManager::LoadIntermediate(const std::string& filepath)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(filepath, aiProcess_Triangulate);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		FATAL_ERROR(std::string("ASSIMP::" + std::string(import.GetErrorString())).c_str());
	}

	std::vector<std::shared_ptr<Mesh>> meshes;
	for (size_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
	{
		std::vector<float> vertices;
		std::vector<uint32_t> indices;
		aiMesh* mesh = scene->mMeshes[meshIndex];

		for (size_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex++)
		{
			aiVector3D vertex = mesh->mVertices[vertexIndex];
			vertices.emplace_back(vertex.x);
			vertices.emplace_back(vertex.y);
			vertices.emplace_back(vertex.z);
			aiVector3D normal = mesh->mNormals[vertexIndex];
			vertices.emplace_back(normal.x);
			vertices.emplace_back(normal.y);
			vertices.emplace_back(normal.z);
			aiVector3D uv = mesh->mTextureCoords[0][vertexIndex];
			vertices.emplace_back(uv.x);
			vertices.emplace_back(uv.y);
		}

		for (size_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++)
		{
			for (size_t i = 0; i < mesh->mFaces[faceIndex].mNumIndices; i++)
			{
				indices.emplace_back(mesh->mFaces[faceIndex].mIndices[i]);
			}
		}

		std::string meshName = mesh->mName.C_Str();
		meshes.emplace_back(std::make_shared<Mesh>(meshName,
			Utils::ExtractDirectoryFromFilePath(filepath) + "/" + meshName + "." + FileFormats::Mesh(),
			vertices,
			indices));
	}

	return meshes;
}

void MeshManager::ShutDown()
{
	m_MeshesByFilepath.clear();
}

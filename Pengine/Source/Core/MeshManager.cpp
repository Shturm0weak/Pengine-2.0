#include "MeshManager.h"

#include "TextureManager.h"
#include "MaterialManager.h"
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

std::unordered_map<std::shared_ptr<Material>, std::vector<std::shared_ptr<Mesh>>>
MeshManager::GenerateMeshes(const std::string& filepath)
{
	std::unordered_map<std::shared_ptr<Material>, std::vector<std::shared_ptr<Mesh>>> meshesByMaterial = LoadIntermediate(filepath);

	if (meshesByMaterial.size() > 1)
	{
		std::string meshesFilepath = Utils::EraseFileFormat(filepath) + "." + FileFormats::Meshes();
		Serializer::SerializeMeshes(meshesFilepath, meshesByMaterial);
	}

	return meshesByMaterial;
}

std::unordered_map<std::shared_ptr<Material>, std::vector<std::shared_ptr<Mesh>>>
MeshManager::LoadIntermediate(const std::string& filepath)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(filepath, aiProcess_Triangulate);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		FATAL_ERROR(std::string("ASSIMP::" + std::string(import.GetErrorString())).c_str());
	}

	std::unordered_map<size_t, std::shared_ptr<Material>> materialsByIndex;
	for (size_t materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++)
	{
		aiMaterial* aiMaterial = scene->mMaterials[materialIndex];

		aiString aiMaterialName;
		aiMaterial->Get(AI_MATKEY_NAME, aiMaterialName);

		std::string materialName = std::string(aiMaterialName.C_Str());
		std::string materialFilepath = Utils::ExtractDirectoryFromFilePath(filepath) + "/" + materialName + ".mat";

		std::shared_ptr<Material> meshBaseMaterial = MaterialManager::GetInstance().LoadMaterial("Materials/MeshBase.mat");
		std::shared_ptr<Material> material = MaterialManager::GetInstance().Clone(materialName, materialFilepath, meshBaseMaterial);

		aiColor3D aiColor(1.0f, 1.0f, 1.0f);
		//aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor);
		glm::vec4 color = { aiColor.r, aiColor.g, aiColor.b, 1.0f };
		material->SetValue("material", "color", color);

		uint32_t numTextures = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
		aiString aiTextureName;
		if (numTextures > 0)
		{
			aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), aiTextureName);
			std::string textureFilepath = aiTextureName.C_Str();
			textureFilepath = Utils::Replace(textureFilepath, '\\', '/');
			textureFilepath = Utils::ExtractDirectoryFromFilePath(filepath) + "/" + textureFilepath;

			material->SetTexture("albedo", TextureManager::GetInstance().Load(textureFilepath));
		}

		Material::Save(material);

		materialsByIndex.emplace(materialIndex, material);
	}

	std::unordered_map<std::shared_ptr<Material>, std::vector<std::shared_ptr<Mesh>>> meshesByMaterial;
	for (size_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
	{
		std::vector<float> vertices;
		std::vector<uint32_t> indices;
		aiMesh* aiMesh = scene->mMeshes[meshIndex];

		std::string defaultMeshName = aiMesh->mName.C_Str();
		std::string meshName = meshName;
		std::string meshFilepath = Utils::ExtractDirectoryFromFilePath(filepath) + "/" + meshName + "." + FileFormats::Mesh();

		int containIndex = 0;
		while (GetMesh(meshFilepath))
		{
			meshName = defaultMeshName + "_" + std::to_string(containIndex);
			meshFilepath = Utils::ExtractDirectoryFromFilePath(filepath) + "/" + meshName + "." + FileFormats::Mesh();
			containIndex++;
		}

		for (size_t vertexIndex = 0; vertexIndex < aiMesh->mNumVertices; vertexIndex++)
		{
			aiVector3D vertex = aiMesh->mVertices[vertexIndex];
			vertices.emplace_back(vertex.x);
			vertices.emplace_back(vertex.y);
			vertices.emplace_back(vertex.z);
			aiVector3D normal = aiMesh->mNormals[vertexIndex];
			vertices.emplace_back(normal.x);
			vertices.emplace_back(normal.y);
			vertices.emplace_back(normal.z);

			if (aiMesh->mNumUVComponents[0] == 2)
			{
				aiVector3D uv = aiMesh->mTextureCoords[0][vertexIndex];
				vertices.emplace_back(uv.x);
				vertices.emplace_back(uv.y);
			}
		}

		for (size_t faceIndex = 0; faceIndex < aiMesh->mNumFaces; faceIndex++)
		{
			for (size_t i = 0; i < aiMesh->mFaces[faceIndex].mNumIndices; i++)
			{
				indices.emplace_back(aiMesh->mFaces[faceIndex].mIndices[i]);
			}
		}

		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(meshName,
			meshFilepath,
			vertices,
			indices);

		meshesByMaterial[materialsByIndex[aiMesh->mMaterialIndex]].emplace_back(mesh);

		m_MeshesByFilepath.emplace(mesh->GetFilepath(), mesh);
		Serializer::SerializeMesh(Utils::ExtractDirectoryFromFilePath(mesh->GetFilepath()), mesh);
	}

	return meshesByMaterial;
}

void MeshManager::ShutDown()
{
	m_MeshesByFilepath.clear();
}

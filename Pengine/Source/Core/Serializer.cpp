#include "Serializer.h"

#include "Logger.h"
#include "FileFormatNames.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "RenderPassManager.h"

#include "../Utils/Utils.h"

#include "yaml-cpp/yaml.h"

#include <filesystem>
#include <fstream>
#include <sstream>

using namespace Pengine;

namespace YAML
{

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::ivec2>
	{
		static Node encode(const glm::ivec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::ivec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<int>();
			rhs.y = node[1].as<int>();
			return true;
		}
	};

	template<>
	struct convert<glm::ivec3>
	{
		static Node encode(const glm::ivec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::ivec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<int>();
			rhs.y = node[1].as<int>();
			rhs.z = node[2].as<int>();
			return true;
		}
	};

	template<>
	struct convert<glm::ivec4>
	{
		static Node encode(const glm::ivec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::ivec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<int>();
			rhs.y = node[1].as<int>();
			rhs.z = node[2].as<int>();
			rhs.w = node[3].as<int>();
			return true;
		}
	};

	template<>
	struct convert<glm::dvec2>
	{
		static Node encode(const glm::dvec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::dvec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<double>();
			rhs.y = node[1].as<double>();
			return true;
		}
	};

	template<>
	struct convert<glm::dvec3>
	{
		static Node encode(const glm::dvec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::dvec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<double>();
			rhs.y = node[1].as<double>();
			rhs.z = node[2].as<double>();
			return true;
		}
	};

	template<>
	struct convert<glm::dvec4>
	{
		static Node encode(const glm::dvec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::dvec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<double>();
			rhs.y = node[1].as<double>();
			rhs.z = node[2].as<double>();
			rhs.w = node[3].as<double>();
			return true;
		}
	};
}

YAML::Emitter& operator<<(YAML::Emitter& out, glm::vec2& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, glm::vec3& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, glm::vec4& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, glm::ivec2& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, glm::ivec3& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, glm::ivec4& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, glm::dvec2& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, glm::dvec3& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, glm::dvec4& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, float* v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq <<
		v[0] << v[1] << v[2] << v[3] <<
		v[4] << v[5] << v[6] << v[7] <<
		v[8] << v[9] << v[10] << v[11] <<
		v[12] << v[13] << v[14] << v[15] <<
		YAML::EndSeq;
	return out;
}

EngineConfig Serializer::DeserializeEngineConfig(const std::string& filepath)
{
	if (filepath.empty() || filepath == none)
	{
		FATAL_ERROR(filepath + ":Failed to load engine config! Filepath is incorrect!")
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!")
	}

	EngineConfig engineConfig;
	if (YAML::Node graphicsAPIData = data["GraphicsAPI"])
	{
		engineConfig.graphicsAPI = static_cast<GraphicsAPI>(graphicsAPIData.as<int>());
	}
	else
	{
		engineConfig.graphicsAPI = GraphicsAPI::Software;
	}

	Logger::Log("Engine config has been loaded!", BOLDGREEN);
	Logger::Log("Graphics API:" + std::to_string((int)engineConfig.graphicsAPI));

	return engineConfig;
}

void Serializer::GenerateFilesUUID(const std::filesystem::path& directory)
{
	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (!entry.is_directory())
		{
			const std::string filepath = entry.path().string();
			const std::string metaFilepath = filepath + ".meta";

			if (!Utils::FindUuid(filepath).empty())
			{
				continue;
			}

			if (FileFormats::IsAsset(Utils::GetFileFormat(filepath)))
			{
				if (!std::filesystem::exists(metaFilepath))
				{
					GenerateFileUUID(filepath);
				}
				else
				{
					std::ifstream stream(metaFilepath);
					std::stringstream stringStream;

					stringStream << stream.rdbuf();

					stream.close();

					YAML::Node data = YAML::LoadMesh(stringStream.str());
					if (!data)
					{
						FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!")
					}

					UUID uuid;
					YAML::Node uuidData = data["UUID"];
					if (!uuidData)
					{
						FATAL_ERROR(filepath + ":Failed to load meta file!")
					}
					else
					{
						uuid = uuidData.as<std::string>();
					}

					filepathByUuid[uuid] = filepath;
					uuidByFilepath[filepath] = uuid;
				}
			}
		}
	}
}

const std::string Serializer::GenerateFileUUID(const std::string& filepath)
{
	std::string fullFilepath = Utils::GetFullFilepath(filepath);

	UUID uuid = Utils::FindUuid(fullFilepath);
	if (!uuid.Get().empty())
	{
		return uuid;
	}

	YAML::Emitter out;

	out << YAML::BeginMap;

	uuid.Generate();
	out << YAML::Key << "UUID" << YAML::Value << uuid;

	out << YAML::EndMap;

	std::ofstream fout(fullFilepath + ".meta");
	fout << out.c_str();
	fout.close();

	filepathByUuid[uuid] = fullFilepath;
	uuidByFilepath[fullFilepath] = uuid;

	return uuid;
}

std::vector<Pipeline::CreateInfo> Serializer::LoadBaseMaterial(const std::string& filepath)
{
	if (!std::filesystem::exists(filepath))
	{
		FATAL_ERROR(filepath + ":Failed to load! The file doesn't exist")
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!")
	}

	YAML::Node materialData = data["Basemat"];
	if (!materialData)
	{
		FATAL_ERROR(filepath + ":Base material file doesn't contain identification line or is empty!")
	}

	std::vector<Pipeline::CreateInfo> pipelineCreateInfos;
	for (const auto& pipelineData : materialData["Pipelines"])
	{
		Pipeline::CreateInfo pipelineCreateInfo{};
		if (const auto& renderPassData = pipelineData["RenderPass"])
		{
			pipelineCreateInfo.renderPass = RenderPassManager::GetInstance().GetRenderPass(
				renderPassData.as<std::string>());
		}

		if (const auto& depthTestData = pipelineData["DepthTest"])
		{
			pipelineCreateInfo.depthTest = depthTestData.as<bool>();
		}

		if (const auto& depthWriteData = pipelineData["DepthWrite"])
		{
			pipelineCreateInfo.depthWrite = depthWriteData.as<bool>();
		}

		if (const auto& cullModeData = pipelineData["CullMode"])
		{
			const std::string& cullMode = cullModeData.as<std::string>();
			if (cullMode == "Front")
			{
				pipelineCreateInfo.cullMode = Pipeline::CullMode::FRONT;
			}
			else if(cullMode == "Back")
			{
				pipelineCreateInfo.cullMode = Pipeline::CullMode::BACK;
			}
			else if (cullMode == "FrontAndBack")
			{
				pipelineCreateInfo.cullMode = Pipeline::CullMode::FRONT_AND_BACK;
			}
			else if (cullMode == "None")
			{
				pipelineCreateInfo.cullMode = Pipeline::CullMode::NONE;
			}
		}

		if (const auto& polygonModeData = pipelineData["PolygonMode"])
		{
			const std::string& polygonMode = polygonModeData.as<std::string>();
			if (polygonMode == "Fill")
			{
				pipelineCreateInfo.polygonMode = Pipeline::PolygonMode::FILL;
			}
			else if (polygonMode == "Line")
			{
				pipelineCreateInfo.polygonMode = Pipeline::PolygonMode::LINE;
			}
		}

		if (const auto& vertexData = pipelineData["Vertex"])
		{
			pipelineCreateInfo.vertexFilepath = vertexData.as<std::string>();
		}

		if (const auto& fragmentData = pipelineData["Fragment"])
		{
			pipelineCreateInfo.fragmentFilepath = fragmentData.as<std::string>();
		}

		for (const auto& uniformData : pipelineData["Uniforms"])
		{
			uint32_t location;
			if (const auto& bindingData = uniformData["Binding"])
			{
				location = bindingData.as<uint32_t>();
			}

			UniformLayout::Binding binding{};
			if (const auto& typeData = uniformData["Type"])
			{
				const std::string& type = typeData.as<std::string>();
				if (type == "SamplerArray")
				{
					binding.type = UniformLayout::Type::SAMPLER_ARRAY;
					if (const auto& countData = uniformData["Count"])
					{
						binding.count = countData.as<int>();
					}
					else
					{
						FATAL_ERROR(filepath + ":No count field is set for sampler array!")
					}
				}
				else if (type == "Sampler")
				{
					binding.type = UniformLayout::Type::SAMPLER;
				}
				else if (type == "Buffer")
				{
					binding.type = UniformLayout::Type::BUFFER;
				}
			}

			if (const auto& nameData = uniformData["Name"])
			{
				binding.name = nameData.as<std::string>();
			}

			size_t offset = 0;
			for (const auto& valueData : uniformData["Values"])
			{
				UniformLayout::Variable value;

				if (const auto& typeData = valueData["Type"])
				{
					value.type = typeData.as<std::string>();
				}

				if (const auto& nameData = valueData["Name"])
				{
					value.name = nameData.as<std::string>();
				}

				value.offset = offset;

				binding.values.emplace_back(value);
				offset += Utils::StringTypeToSize(value.type);
			}

			for (const auto& stageData : uniformData["Stages"])
			{
				const std::string& stage = stageData.as<std::string>();
				if (stage == "Fragment")
				{
					binding.stages.emplace_back(UniformLayout::Stage::FRAGMENT);
				}
				else if (stage == "Vertex")
				{
					binding.stages.emplace_back(UniformLayout::Stage::VERTEX);
				}
			}

			if (const auto& inputRateData = uniformData["InputRate"])
			{
				std::string inputRate = inputRateData.as<std::string>();
				if (inputRate == "Per BaseMaterial")
				{
					pipelineCreateInfo.uniformBindings[location] = binding;
				}
				else if (inputRate == "Per Material")
				{
					pipelineCreateInfo.childUniformBindings[location] = binding;
				}
			}
		}

		for (const auto& colorBlendStateData : pipelineData["ColorBlendStates"])
		{
			if (const auto& blendEnabledData = colorBlendStateData["BlendEnabled"])
			{
				pipelineCreateInfo.colorBlendStateAttachments.emplace_back(
					Pipeline::BlendStateAttachment{ blendEnabledData.as<bool>() });
			}
		}

		pipelineCreateInfos.emplace_back(pipelineCreateInfo);
	}

	return pipelineCreateInfos;
}

Material::CreateInfo Serializer::LoadMaterial(const std::string& filepath)
{
	if (!std::filesystem::exists(filepath))
	{
		FATAL_ERROR(filepath + ":Failed to load! The file doesn't exist")
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!")
	}

	YAML::Node materialData = data["Mat"];
	if (!materialData)
	{
		FATAL_ERROR(filepath + ":Base material file doesn't contain identification line or is empty!")
	}

	Material::CreateInfo createInfo;

	if (const auto& baseMaterialData = materialData["Basemat"])
	{
		createInfo.baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(baseMaterialData.as<std::string>());

		if (!createInfo.baseMaterial)
		{
			FATAL_ERROR(baseMaterialData.as<std::string>() + ":There is no such basemat!")
		}
	}

	for (const auto& pipelineData : materialData["Pipelines"])
	{
		std::string renderPass = none;
		if (const auto& renderPassData = pipelineData["RenderPass"])
		{
			renderPass = renderPassData.as<std::string>();
		}
		else
		{
			FATAL_ERROR(filepath + ":There is no RenderPass field!")
		}

		Material::RenderPassInfo renderPassInfo{};

		for (const auto & uniformData : pipelineData["Uniforms"])
		{
			std::string uniformName;
			if (const auto& nameData = uniformData["Name"])
			{
				uniformName = nameData.as<std::string>();
			}
			UniformLayout::Binding binding = createInfo.baseMaterial->GetPipeline(renderPass)->
				GetChildUniformLayout()->GetBindingByName(uniformName);

			if (binding.type == UniformLayout::Type::BUFFER)
			{
				Material::UniformBufferInfo uniformBufferInfo{};

				for (const auto& valueData : uniformData["Values"])
				{
					std::string valueName;
					if (const auto& nameData = valueData["Name"])
					{
						valueName = nameData.as<std::string>();
					}

					if (std::optional<UniformLayout::Variable> value = binding.GetValue(valueName))
					{
						if (const auto& bufferValueData = valueData["Value"])
						{
							if (value->type == "int")
							{
								uniformBufferInfo.intValuesByName.emplace(valueName, bufferValueData.as<int>());
							}
							else if (value->type == "sampler")
							{
								uniformBufferInfo.samplerValuesByName.emplace(valueName, bufferValueData.as<std::string>());
							}
							else if (value->type == "float")
							{
								uniformBufferInfo.floatValuesByName.emplace(valueName, bufferValueData.as<float>());
							}
							else if (value->type == "vec2")
							{
								uniformBufferInfo.vec2ValuesByName.emplace(valueName, bufferValueData.as<glm::vec2>());
							}
							else if (value->type == "vec3")
							{
								uniformBufferInfo.vec3ValuesByName.emplace(valueName, bufferValueData.as<glm::vec3>());
							}
							else if (value->type == "vec4")
							{
								uniformBufferInfo.vec4ValuesByName.emplace(valueName, bufferValueData.as<glm::vec4>());
							}
							else if (value->type == "color")
							{
								uniformBufferInfo.vec4ValuesByName.emplace(valueName, bufferValueData.as<glm::vec4>());
							}
						}
					}
				}

				renderPassInfo.uniformBuffersByName.emplace(uniformName, uniformBufferInfo);
			}
			else if (binding.type == UniformLayout::Type::SAMPLER)
			{
				if (const auto& nameData = uniformData["Value"])
				{
					const std::string textureFilepath = nameData.as<std::string>();
					renderPassInfo.texturesByName.emplace(uniformName, textureFilepath);
				}
			}
		}
		createInfo.renderPassInfo.emplace(renderPass, renderPassInfo);
	}

	return createInfo;
}

void Serializer::SerializeMesh(const std::string& directory, std::shared_ptr<Mesh> mesh)
{
	std::string meshName = mesh->GetName();
	std::string meshFilepath = mesh->GetFilepath();

	size_t dataSize = mesh->GetVertexCount() * sizeof(float) +
		mesh->GetIndexCount() * sizeof(uint32_t) +
		mesh->GetName().size() +
		mesh->GetFilepath().size() +
		4 * 4; // Plus 4 numbers for these 4 fields sizes.

	int offset = 0;

	char* data = new char[dataSize];

	// Name.
	{
		Utils::GetValue<int>(data, offset) = (int)meshName.size();
		offset += sizeof(int);

		memcpy(&Utils::GetValue<char>(data, offset), meshName.data(), meshName.size());
		offset += meshName.size();
	}

	// Filepath.
	{
		Utils::GetValue<int>(data, offset) = (int)meshFilepath.size();
		offset += sizeof(int);
		memcpy(&Utils::GetValue<char>(data, offset), meshFilepath.data(), meshFilepath.size());
		offset += meshFilepath.size();
	}

	// Vertices.
	{
		Utils::GetValue<int>(data, offset) = mesh->GetVertexCount();
		offset += sizeof(int);

		std::vector<float> vertices = mesh->GetRawVertices();
		memcpy(&Utils::GetValue<char>(data, offset), vertices.data(), mesh->GetVertexCount() * sizeof(float));
		offset += mesh->GetVertexCount() * sizeof(float);
	}

	// Indices.
	{
		Utils::GetValue<int>(data, offset) = mesh->GetIndexCount();
		offset += sizeof(int);

		std::vector<uint32_t> indices = mesh->GetRawIndices();
		memcpy(&Utils::GetValue<char>(data, offset), indices.data(), mesh->GetIndexCount() * sizeof(uint32_t));
		offset += mesh->GetIndexCount() * sizeof(uint32_t);
	}

	std::ofstream out(directory + "/" + meshName + "." + FileFormats::Mesh(), std::ostream::binary);

	out.write(data, dataSize);

	delete[] data;

	out.close();
}

std::shared_ptr<Mesh> Serializer::DeserializeMesh(const std::string& filepath)
{
	if (!std::filesystem::exists(filepath))
	{
		Logger::Error(filepath + ": doesn't exist!");
		return nullptr;
	}

	if (FileFormats::Mesh() != Utils::GetFileFormat(filepath))
	{
		Logger::Error(filepath + ": is not mesh asset!");
		return nullptr;
	}

	std::ifstream in(filepath, std::ifstream::binary);

	in.seekg(0, in.end);
	int size = in.tellg();
	in.seekg(0, in.beg);

	char* data = new char[size];

	in.read(data, size);

	in.close();

	int offset = 0;

	std::string meshName;
	// Name.
	{
		const int meshNameSize = Utils::GetValue<int>(data, offset);
		offset += sizeof(int);

		meshName.resize(meshNameSize);
		memcpy(meshName.data(), &Utils::GetValue<char>(data, offset), meshNameSize);
		offset += meshNameSize;
	}

	std::string meshFilepath;
	// Filepath.
	{
		const int meshFilepathSize = Utils::GetValue<int>(data, offset);
		offset += sizeof(int);

		meshFilepath.resize(meshFilepathSize);
		memcpy(meshFilepath.data(), &Utils::GetValue<char>(data, offset), meshFilepathSize);
		offset += meshFilepathSize;
	}

	std::vector<float> vertices;
	// Vertices.
	{
		const int verticesSize = Utils::GetValue<int>(data, offset);
		offset += sizeof(int);

		vertices.resize(verticesSize);
		memcpy(vertices.data(), &Utils::GetValue<char>(data, offset), verticesSize * sizeof(float));
		offset += verticesSize * sizeof(float);
	}

	std::vector<uint32_t> indices;
	// Indices.
	{
		const int indicesSize = Utils::GetValue<int>(data, offset);
		offset += sizeof(int);

		indices.resize(indicesSize);
		memcpy(indices.data(), &Utils::GetValue<char>(data, offset), indicesSize * sizeof(uint32_t));
		offset += indicesSize * sizeof(uint32_t);
	}

	delete[] data;

	return std::make_shared<Mesh>(meshName, meshFilepath, vertices, indices);
}

void Serializer::SerializeMeshes(const std::string& filepath, std::vector<std::shared_ptr<Mesh>> meshes)
{
	std::vector<std::string> uuids;
	for (std::shared_ptr<Mesh> mesh : meshes)
	{
		std::string uuid = Utils::FindUuid(mesh->GetFilepath());
		if (uuid.empty())
		{
			uuid = GenerateFileUUID(mesh->GetFilepath());
		}

		uuids.emplace_back(uuid);
	}

	YAML::Emitter out;

	out << YAML::BeginMap;

	out << YAML::Key << "Meshes" << YAML::Value << uuids;

	out << YAML::EndMap;

	std::ofstream fout(filepath);
	fout << out.c_str();
	fout.close();
}

std::vector<std::shared_ptr<Mesh>> Serializer::DeserializeMeshes(const std::string& filepath)
{
	std::vector<std::shared_ptr<Mesh>> meshes;

	if (filepath.empty() || filepath == none || Utils::GetFileFormat(filepath) != FileFormats::Meshes())
	{
		Logger::Error(filepath + ":Failed to load meshes! Filepath is incorrect!");
		return meshes;
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!")
	}

	if (YAML::Node meshesData = data["Meshes"])
	{
		for (const auto& meshUuidData : meshesData)
		{
			std::string uuid = meshUuidData.as<std::string>();
			std::string filepath = Utils::Find(uuid, filepathByUuid);
			if (!filepath.empty())
			{
				meshes.emplace_back(MeshManager::GetInstance().LoadMesh(filepath));
			}
		}
	}

	return meshes;
}

void Serializer::SerializeShaderCache(std::string filepath, const std::string& code)
{
	const char* directory = "Shaders/Cache/";
	filepath = directory + Utils::RemoveDirectoryFromFilePath(filepath) + "." + FileFormats::Spv();
	std::ofstream out(filepath, std::ostream::binary);
	out.write(code.data(), code.size());
	out.close();
}

std::string Serializer::DeserializeShaderCache(std::string filepath)
{
	if (filepath.empty())
	{
		return {};
	}

	const char* directory = "Shaders/Cache/";
	filepath = directory + Utils::RemoveDirectoryFromFilePath(filepath) + "." + FileFormats::Spv();
	if (std::filesystem::exists(filepath))
	{
		std::ifstream in(filepath, std::ifstream::binary);

		in.seekg(0, in.end);
		int size = in.tellg();
		in.seekg(0, in.beg);

		std::string data;
		data.resize(size);

		in.read(data.data(), size);

		in.close();

		return data;
	}

	return {};
}

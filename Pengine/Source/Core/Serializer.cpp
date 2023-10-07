#include "Serializer.h"

#include "Logger.h"
#include "FileFormatNames.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "RenderPassManager.h"
#include "TextureManager.h"
#include "SceneManager.h"

#include "../Components/Transform.h"
#include "../Components/Renderer3D.h"

#include "../Utils/Utils.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

	Logger::Log("BaseMaterial:" + filepath + " has been deserialized!", GREEN);

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

	Logger::Log("Material:" + filepath + " has been deserialized!", GREEN);

	return createInfo;
}

void Serializer::SerializeMaterial(std::shared_ptr<Material> material)
{
	YAML::Emitter out;

	out << YAML::BeginMap;

	out << YAML::Key << "Mat";

	out << YAML::BeginMap;

	out << YAML::Key << "Basemat" << YAML::Value << material->GetBaseMaterial()->GetFilepath();

	out << YAML::Key << "Pipelines";

	out << YAML::BeginSeq;

	for (const auto& [renderPass, pipeline] : material->GetBaseMaterial()->GetPipelinesByRenderPass())
	{
		out << YAML::BeginMap;

		out << YAML::Key << "RenderPass" << YAML::Value << renderPass;

		out << YAML::Key << "Uniforms";

		out << YAML::BeginSeq;

		for (const auto& [location, binding] : pipeline->GetChildUniformLayout()->GetBindingsByLocation())
		{
			out << YAML::BeginMap;

			out << YAML::Key << "Name" << YAML::Value << binding.name;

			if (binding.type == UniformLayout::Type::SAMPLER)
			{
				// TODO: Better to use uuid for assets instead of filepaths!
				out << YAML::Key << "Value" << YAML::Value << material->GetTexture(binding.name)->GetFilepath();
			}
			else if (binding.type == UniformLayout::Type::BUFFER)
			{
				std::shared_ptr<Buffer> buffer = pipeline->GetBuffer(binding.name);
				void* data = (char*)buffer->GetData() + buffer->GetInstanceSize() * material->GetIndex();

				out << YAML::Key << "Values";

				out << YAML::BeginSeq;

				for (const auto& value : binding.values)
				{
					out << YAML::BeginMap;

					out << YAML::Key << "Name" << YAML::Value << value.name;

					if (value.type == "vec2")
					{
						out << YAML::Key << "Value" << YAML::Value << Utils::GetValue<glm::vec2>(data, value.offset);
					}
					else if (value.type == "vec3")
					{
						out << YAML::Key << "Value" << YAML::Value << Utils::GetValue<glm::vec3>(data, value.offset);
					}
					else if (value.type == "vec4" || value.type == "color")
					{
						out << YAML::Key << "Value" << YAML::Value << Utils::GetValue<glm::vec4>(data, value.offset);
					}
					else if (value.type == "float")
					{
						out << YAML::Key << "Value" << YAML::Value << Utils::GetValue<float>(data, value.offset);
					}
					else if (value.type == "int")
					{
						out << YAML::Key << "Value" << YAML::Value << Utils::GetValue<int>(data, value.offset);
					}
					else if (value.type == "sampler")
					{
						// TODO: Fill the value when figure out how to handle sampler arrays!
					}

					out << YAML::EndMap;
				}

				out << YAML::EndSeq;
			}

			out << YAML::EndMap;
		}

		out << YAML::EndSeq;

		out << YAML::EndMap;
	}

	out << YAML::EndSeq;

	out << YAML::EndMap;

	out << YAML::EndMap;

	std::ofstream fout(material->GetFilepath());
	fout << out.c_str();
	fout.close();

	std::string uuid = Utils::FindUuid(material->GetFilepath());
	if (uuid.empty())
	{
		uuid = GenerateFileUUID(material->GetFilepath());
	}

	Logger::Log("Material:" + material->GetFilepath() + " has been serialized!", GREEN);
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

		// Potential optimization of using 2 bit instead of 4 bit.
		std::vector<uint32_t> indices = mesh->GetRawIndices();
		memcpy(&Utils::GetValue<char>(data, offset), indices.data(), mesh->GetIndexCount() * sizeof(uint32_t));
		offset += mesh->GetIndexCount() * sizeof(uint32_t);
	}

	const std::string outMeshFilepath = directory + "/" + meshName + "." + FileFormats::Mesh();
	std::ofstream out(outMeshFilepath, std::ostream::binary);

	out.write(data, dataSize);

	delete[] data;

	out.close();

	std::string uuid = Utils::FindUuid(mesh->GetFilepath());
	if (uuid.empty())
	{
		uuid = GenerateFileUUID(mesh->GetFilepath());
	}

	Logger::Log("Mesh:" + outMeshFilepath + " has been serialized!", GREEN);
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

	Logger::Log("Mesh:" + filepath + " has been deserialized!", GREEN);

	return std::make_shared<Mesh>(meshName, filepath, vertices, indices);
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

std::unordered_map<std::shared_ptr<Material>, std::vector<std::shared_ptr<Mesh>>>
Serializer::LoadIntermediate(const std::string& filepath)
{
	const std::string directory = Utils::ExtractDirectoryFromFilePath(filepath);

	Assimp::Importer import;
	const auto importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices |
		aiProcess_OptimizeMeshes | aiProcess_RemoveRedundantMaterials | aiProcess_ImproveCacheLocality |
		aiProcess_JoinIdenticalVertices | aiProcess_GenUVCoords | aiProcess_SortByPType | aiProcess_FindInstances |
		aiProcess_ValidateDataStructure | aiProcess_FindDegenerates | aiProcess_FindInvalidData;
	const aiScene* scene = import.ReadFile(filepath, importFlags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		FATAL_ERROR(std::string("ASSIMP::" + std::string(import.GetErrorString())).c_str());
	}

	std::unordered_map<size_t, std::shared_ptr<Material>> materialsByIndex;
	for (size_t materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++)
	{
		materialsByIndex[materialIndex] = GenerateMaterial(scene->mMaterials[materialIndex], directory);
	}

	std::unordered_map<size_t, std::shared_ptr<Mesh>> meshesByIndex;
	std::unordered_map<std::shared_ptr<Mesh>, std::shared_ptr<Material>> materialsByMeshes;
	for (size_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
	{
		aiMesh* aiMesh = scene->mMeshes[meshIndex];
		std::shared_ptr<Mesh> mesh = GenerateMesh(aiMesh, directory);
		meshesByIndex[meshIndex] = mesh;
		materialsByMeshes[mesh] = materialsByIndex[aiMesh->mMaterialIndex];
	}

	SceneManager::GetInstance().Create("GenerateGameObject", "GenerateGameObject");

	GameObject* root = GenerateGameObject(scene->mRootNode, meshesByIndex, materialsByMeshes);

	// Consider that only a hierarchy of game objects can be serilized as a prefab.
	if (root->GetChilds().size() > 0)
	{
		SerializePrefab(directory + "/" + root->GetName() + "." + FileFormats::Prefab(), root);
	}

	SceneManager::GetInstance().Delete("GenerateGameObject");

	return {};
}

std::shared_ptr<Mesh> Serializer::GenerateMesh(aiMesh* aiMesh, const std::string& directory)
{
	std::vector<float> vertices;
	std::vector<uint32_t> indices;

	std::string defaultMeshName = aiMesh->mName.C_Str();
	std::string meshName = defaultMeshName;
	std::string meshFilepath = directory + "/" + meshName + "." + FileFormats::Mesh();

	int containIndex = 0;
	while (MeshManager::GetInstance().GetMesh(meshFilepath))
	{
		meshName = defaultMeshName + "_" + std::to_string(containIndex);
		meshFilepath = directory + "/" + meshName + "." + FileFormats::Mesh();
		containIndex++;
	}

	for (size_t vertexIndex = 0; vertexIndex < aiMesh->mNumVertices; vertexIndex++)
	{
		aiVector3D vertex = aiMesh->mVertices[vertexIndex];
		vertices.emplace_back(vertex.x);
		vertices.emplace_back(vertex.y);
		vertices.emplace_back(vertex.z);

		if (aiMesh->mNumUVComponents[0] == 2)
		{
			aiVector3D uv = aiMesh->mTextureCoords[0][vertexIndex];
			vertices.emplace_back(uv.x);
			vertices.emplace_back(uv.y);
		}
		else
		{
			vertices.emplace_back(0.0f);
			vertices.emplace_back(0.0f);
		}

		aiVector3D normal = aiMesh->mNormals[vertexIndex];
		vertices.emplace_back(normal.x);
		vertices.emplace_back(normal.y);
		vertices.emplace_back(normal.z);

		if (aiMesh->HasTangentsAndBitangents())
		{
			// Tangent.
			aiVector3D tangent = aiMesh->mTangents[vertexIndex];
			vertices.emplace_back(tangent.x);
			vertices.emplace_back(tangent.y);
			vertices.emplace_back(tangent.z);

			// Bitngent.
			aiVector3D bitangent = aiMesh->mBitangents[vertexIndex];
			vertices.emplace_back(bitangent.x);
			vertices.emplace_back(bitangent.y);
			vertices.emplace_back(bitangent.z);
		}
		else
		{
			// Tangent.
			vertices.emplace_back(0.0f);
			vertices.emplace_back(0.0f);
			vertices.emplace_back(0.0f);

			// Bitangent.
			vertices.emplace_back(0.0f);
			vertices.emplace_back(0.0f);
			vertices.emplace_back(0.0f);
		}
	}

	const size_t vertexOffset = sizeof(Vertex) / sizeof(float);
	const size_t tangentOffset = offsetof(Vertex, tangent) / sizeof(float);
	const size_t bitangentOffset = offsetof(Vertex, bitangent) / sizeof(float);
	for (size_t faceIndex = 0; faceIndex < aiMesh->mNumFaces; faceIndex++)
	{
		for (size_t i = 0; i < aiMesh->mFaces[faceIndex].mNumIndices; i++)
		{
			indices.emplace_back(aiMesh->mFaces[faceIndex].mIndices[i]);
		}

		if (!aiMesh->HasTangentsAndBitangents())
		{
			size_t i0 = aiMesh->mFaces[faceIndex].mIndices[0];
			size_t i1 = aiMesh->mFaces[faceIndex].mIndices[1];
			size_t i2 = aiMesh->mFaces[faceIndex].mIndices[2];

			glm::vec3 position1 = { aiMesh->mVertices[i0].x, aiMesh->mVertices[i0].y, aiMesh->mVertices[i0].z };
			glm::vec3 position2 = { aiMesh->mVertices[i1].x, aiMesh->mVertices[i1].y, aiMesh->mVertices[i1].z };
			glm::vec3 position3 = { aiMesh->mVertices[i2].x, aiMesh->mVertices[i2].y, aiMesh->mVertices[i2].z };

			glm::vec2 uv1 = { aiMesh->mTextureCoords[0][i0].x, aiMesh->mTextureCoords[0][i0].y };
			glm::vec2 uv2 = { aiMesh->mTextureCoords[0][i1].x, aiMesh->mTextureCoords[0][i1].y };
			glm::vec2 uv3 = { aiMesh->mTextureCoords[0][i2].x, aiMesh->mTextureCoords[0][i2].y };

			glm::vec3 edge1 = position2 - position1;
			glm::vec3 edge2 = position3 - position1;
			glm::vec2 deltaUV1 = uv2 - uv1;
			glm::vec2 deltaUV2 = uv3 - uv1;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent, bitangent;

			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent = glm::normalize(tangent);

			bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
			bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
			bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
			bitangent = glm::normalize(bitangent);

			vertices[i0 * vertexOffset + tangentOffset + 0] = tangent.x;
			vertices[i0 * vertexOffset + tangentOffset + 1] = tangent.y;
			vertices[i0 * vertexOffset + tangentOffset + 2] = tangent.z;
			vertices[i0 * vertexOffset + bitangentOffset + 0] = bitangent.x;
			vertices[i0 * vertexOffset + bitangentOffset + 1] = bitangent.y;
			vertices[i0 * vertexOffset + bitangentOffset + 2] = bitangent.z;

			vertices[i1 * vertexOffset + tangentOffset + 0] = tangent.x;
			vertices[i1 * vertexOffset + tangentOffset + 1] = tangent.y;
			vertices[i1 * vertexOffset + tangentOffset + 2] = tangent.z;
			vertices[i1 * vertexOffset + bitangentOffset + 0] = bitangent.x;
			vertices[i1 * vertexOffset + bitangentOffset + 1] = bitangent.y;
			vertices[i1 * vertexOffset + bitangentOffset + 2] = bitangent.z;

			vertices[i2 * vertexOffset + tangentOffset + 0] = tangent.x;
			vertices[i2 * vertexOffset + tangentOffset + 1] = tangent.y;
			vertices[i2 * vertexOffset + tangentOffset + 2] = tangent.z;
			vertices[i2 * vertexOffset + bitangentOffset + 0] = bitangent.x;
			vertices[i2 * vertexOffset + bitangentOffset + 1] = bitangent.y;
			vertices[i2 * vertexOffset + bitangentOffset + 2] = bitangent.z;
		}
	}

	std::shared_ptr<Mesh> mesh = MeshManager::GetInstance().CreateMesh(meshName, meshFilepath, vertices, indices);
	Serializer::SerializeMesh(Utils::ExtractDirectoryFromFilePath(mesh->GetFilepath()), mesh);

	return mesh;
}

std::shared_ptr<Material> Serializer::GenerateMaterial(aiMaterial* aiMaterial, const std::string& directory)
{
	aiString aiMaterialName;
	aiMaterial->Get(AI_MATKEY_NAME, aiMaterialName);

	std::string materialName = std::string(aiMaterialName.C_Str());
	if (materialName == "DefaultMaterial")
	{
		return MaterialManager::GetInstance().LoadMaterial("Materials/MeshBase.mat");
	}

	std::string materialFilepath = directory + "/" + materialName + ".mat";

	std::shared_ptr<Material> meshBaseMaterial = MaterialManager::GetInstance().LoadMaterial("Materials/MeshBase.mat");
	std::shared_ptr<Material> material = MaterialManager::GetInstance().Clone(materialName, materialFilepath, meshBaseMaterial);

	aiColor3D aiColor(0.0f, 0.0f, 0.0f);
	aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor);
	glm::vec4 color = { aiColor.r, aiColor.g, aiColor.b, 1.0f };
	material->SetValue("material", "color", color);

	uint32_t numTextures = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
	aiString aiTextureName;
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), aiTextureName);
		std::string textureFilepath = aiTextureName.C_Str();
		textureFilepath = Utils::Replace(textureFilepath, '\\', '/');
		textureFilepath = directory + "/" + textureFilepath;

		material->SetTexture("albedoTexture", TextureManager::GetInstance().Load(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_NORMALS);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), aiTextureName);
		std::string textureFilepath = aiTextureName.C_Str();
		textureFilepath = Utils::Replace(textureFilepath, '\\', '/');
		textureFilepath = directory + "/" + textureFilepath;

		material->SetTexture("normalTexture", TextureManager::GetInstance().Load(textureFilepath));

		const float useNormalMap = 1.0f;
		glm::vec4 other = { useNormalMap, 0.0f, 0.0f, 0.0f };
		material->SetValue("material", "other", other);
	}

	Material::Save(material);

	return material;
}

GameObject* Serializer::GenerateGameObject(aiNode* assimpNode,
	const std::unordered_map<size_t, std::shared_ptr<Mesh>>& meshesByIndex,
	const std::unordered_map<std::shared_ptr<Mesh>, std::shared_ptr<Material>>& materialsByMeshes)
{
	if (!assimpNode)
	{
		return nullptr;
	}

	std::shared_ptr<Scene> scene = SceneManager::GetInstance().GetSceneByTag("GenerateGameObject");
	GameObject* node = scene->CreateGameObject(assimpNode->mName.C_Str());

	Renderer3D* r3d = nullptr;
	if (assimpNode->mNumMeshes > 0)
	{
		r3d = node->m_ComponentManager.AddComponent<Renderer3D>();
	}

	for (size_t meshIndex = 0; meshIndex < assimpNode->mNumMeshes; meshIndex++)
	{
		auto meshByIndex = meshesByIndex.find(assimpNode->mMeshes[meshIndex]);
		if (meshByIndex != meshesByIndex.end())
		{
			r3d->mesh = meshByIndex->second;
			r3d->material = materialsByMeshes.find(meshByIndex->second)->second;
		}
	}

	for (size_t childIndex = 0; childIndex < assimpNode->mNumChildren; childIndex++)
	{
		aiNode* child = assimpNode->mChildren[childIndex];
		node->AddChild(GenerateGameObject(child, meshesByIndex, materialsByMeshes));
	}

	aiVector3D aiPosition;
	aiVector3D aiRotation;
	aiVector3D aiScale;
	assimpNode->mTransformation.Decompose(aiScale, aiRotation, aiPosition);

	glm::vec3 position = { aiPosition.x, aiPosition.y, aiPosition.z };
	glm::vec3 rotation = { aiRotation.x, aiRotation.y, aiRotation.z };
	glm::vec3 scale = { aiScale.x, aiScale.y, aiScale.z };

	node->m_Transform.Translate(position);
	node->m_Transform.Rotate(rotation);
	node->m_Transform.Scale(scale);

	return node;
}

void Serializer::SerializeGameObject(YAML::Emitter& out, GameObject* gameObject)
{
	out << YAML::BeginMap;

	out << YAML::Key << "UUID" << YAML::Value << gameObject->GetUUID();
	out << YAML::Key << "Name" << YAML::Value << gameObject->GetName();
	out << YAML::Key << "IsEnabled" << YAML::Value << gameObject->IsEnabled();

	SerializeTransform(out, &gameObject->m_Transform);
	SerializeRenderer3D(out, gameObject->m_ComponentManager.GetComponent<Renderer3D>());

	std::vector<std::string> childUUIDs;
	for (GameObject* child : gameObject->GetChilds())
	{
		childUUIDs.emplace_back(child->GetUUID());
	}

	out << YAML::Key << "Childs" << YAML::Value << childUUIDs;

	out << YAML::EndMap;

	for (GameObject* child : gameObject->GetChilds())
	{
		SerializeGameObject(out, child);
	}
}

void Serializer::SerializePrefab(const std::string& filepath, GameObject* gameObject)
{
	YAML::Emitter out;

	out << YAML::BeginSeq;

	SerializeGameObject(out, gameObject);

	out << YAML::EndSeq;

	std::ofstream fout(filepath);
	fout << out.c_str();
	fout.close();
}

GameObject* Serializer::DeserializePrefab(const std::string& filepath, std::shared_ptr<Scene> scene)
{
	if (filepath.empty() || filepath == none || Utils::GetFileFormat(filepath) != FileFormats::Prefab())
	{
		Logger::Error(filepath + ":Failed to load prefab! Filepath is incorrect!");
		return nullptr;
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

	std::unordered_map<GameObject*, std::vector<std::string>> childsByGameObject;
	for (const auto& gameObjectData : data)
	{
		UUID uuid;
		if (const auto& uuidData = gameObjectData["UUID"])
		{
			uuid = uuidData.as<std::string>();
		}

		std::string name;
		if (const auto& nameData = gameObjectData["Name"])
		{
			name = nameData.as<std::string>();
		}

		GameObject* gameObject = scene->CreateGameObject(name, {}, uuid);

		if (const auto& isEnabledData = gameObjectData["IsEnabled"])
		{
			gameObject->SetEnabled(isEnabledData.as<bool>());
		}

		DeserializeTransform(gameObjectData, &gameObject->m_Transform);
		DeserializeRenderer3D(gameObjectData, gameObject);

		auto& childUuids = childsByGameObject[gameObject];
		for (const auto& childData : gameObjectData["Childs"])
		{
			childUuids.emplace_back(childData.as<std::string>());
		}
	}

	for (const auto& [gameObject, childs] : childsByGameObject)
	{
		for (const auto& child : childs)
		{
			gameObject->AddChild(scene->FindGameObjectByUUID(child));
		}
	}

	Logger::Log("Meshes:" + filepath + " has been deserialized!", GREEN);

	return nullptr;
}

void Serializer::SerializeTransform(YAML::Emitter& out, Transform* transform)
{
	if (!transform)
	{
		return;
	}

	out << YAML::Key << "Transform";

	out << YAML::BeginMap;

	out << YAML::Key << "Position" << YAML::Value << transform->GetPosition();
	out << YAML::Key << "Rotation" << YAML::Value << transform->GetRotation();
	out << YAML::Key << "Scale" << YAML::Value << transform->GetScale();
	out << YAML::Key << "FollowOwner" << YAML::Value << transform->GetFollorOwner();

	out << YAML::EndMap;
}

void Serializer::DeserializeTransform(const YAML::Node& in, Transform* transform)
{
	if (!transform)
	{
		return;
	}

	if (const auto& transformData = in["Transform"])
	{
		if (const auto& positionData = transformData["Position"])
		{
			transform->Translate(positionData.as<glm::vec3>());
		}

		if (const auto& rotationData = transformData["Rotation"])
		{
			transform->Rotate(rotationData.as<glm::vec3>());
		}

		if (const auto& scaleData = transformData["Scale"])
		{
			transform->Scale(scaleData.as<glm::vec3>());
		}

		if (const auto& follorOwnerData = transformData["FollowOwner"])
		{
			transform->SetFollowOwner(follorOwnerData.as<bool>());
		}
	}
}

void Serializer::SerializeRenderer3D(YAML::Emitter& out, Renderer3D* r3d)
{
	if (!r3d)
	{
		return;
	}

	out << YAML::Key << "Renderer3D";

	out << YAML::BeginMap;

	out << YAML::Key << "Mesh" << YAML::Value << Utils::FindUuid(r3d->mesh->GetFilepath());
	out << YAML::Key << "Material" << YAML::Value << Utils::FindUuid(r3d->material->GetFilepath());

	out << YAML::EndMap;
}

void Serializer::DeserializeRenderer3D(const YAML::Node& in, GameObject* gameObject)
{
	if (!gameObject)
	{
		return;
	}

	if (const auto& renderer3DData = in["Renderer3D"])
	{
		Renderer3D* r3d = gameObject->m_ComponentManager.AddComponent<Renderer3D>();

		if (const auto& meshData = renderer3DData["Mesh"])
		{
			std::string uuid = meshData.as<std::string>();
			std::shared_ptr<Mesh> mesh = MeshManager::GetInstance().LoadMesh(Utils::GetShortFilepath(Utils::Find(uuid, filepathByUuid)));
			r3d->mesh = mesh;
		}

		if (const auto& materialData = renderer3DData["Material"])
		{
			std::string uuid = materialData.as<std::string>();
			std::shared_ptr<Material> material = MaterialManager::GetInstance().LoadMaterial(Utils::GetShortFilepath(Utils::Find(uuid, filepathByUuid)));
			r3d->material = material;
		}
	}
}

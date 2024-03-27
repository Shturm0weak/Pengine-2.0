#include "Serializer.h"

#include "FileFormatNames.h"
#include "Logger.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "RenderPassManager.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include "ViewportManager.h"

#include "../Components/Camera.h"
#include "../Components/PointLight.h"
#include "../Components/Renderer3D.h"
#include "../Components/Transform.h"

#include "../Utils/Utils.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::ivec2& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::ivec3& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::ivec4& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::dvec2& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::dvec3& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::dvec4& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const float* v)
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
		FATAL_ERROR(filepath + ":Failed to load engine config! Filepath is incorrect!");
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
	}

	EngineConfig engineConfig{};
	if (YAML::Node graphicsAPIData = data["GraphicsAPI"])
	{
		engineConfig.graphicsAPI = static_cast<GraphicsAPI>(graphicsAPIData.as<int>());
	}
	else
	{
		engineConfig.graphicsAPI = GraphicsAPI::Vk;
	}

	Logger::Log("Engine config has been loaded!", BOLDGREEN);
	Logger::Log("Graphics API:" + std::to_string(static_cast<int>(engineConfig.graphicsAPI)));

	return engineConfig;
}

void Serializer::GenerateFilesUUID(const std::filesystem::path& directory)
{
	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (!entry.is_directory())
		{
			const std::string filepath = Utils::GetShortFilepath(entry.path().string());
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
						FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
					}

					UUID uuid;
					if (YAML::Node uuidData = data["UUID"]; !uuidData)
					{
						FATAL_ERROR(filepath + ":Failed to load meta file!");
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

std::string Serializer::GenerateFileUUID(const std::string& filepath)
{
	UUID uuid = Utils::FindUuid(filepath);
	if (!uuid.Get().empty())
	{
		return uuid;
	}

	YAML::Emitter out;

	out << YAML::BeginMap;

	uuid.Generate();
	out << YAML::Key << "UUID" << YAML::Value << uuid;

	out << YAML::EndMap;

	std::ofstream fout(filepath + ".meta");
	fout << out.c_str();
	fout.close();

	filepathByUuid[uuid] = filepath;
	uuidByFilepath[filepath] = uuid;

	return uuid;
}

std::vector<Pipeline::CreateInfo> Serializer::LoadBaseMaterial(const std::string& filepath)
{
	if (!std::filesystem::exists(filepath))
	{
		FATAL_ERROR(filepath + ":Failed to load! The file doesn't exist");
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
	}

	YAML::Node materialData = data["Basemat"];
	if (!materialData)
	{
		FATAL_ERROR(filepath + ":Base material file doesn't contain identification line or is empty!");
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
			if (const std::string& cullMode = cullModeData.as<std::string>(); cullMode == "Front")
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
			if (const std::string& polygonMode = polygonModeData.as<std::string>(); polygonMode == "Fill")
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
				if (const std::string& type = typeData.as<std::string>(); type == "SamplerArray")
				{
					binding.type = UniformLayout::Type::SAMPLER_ARRAY;
					if (const auto& countData = uniformData["Count"])
					{
						binding.count = countData.as<int>();
					}
					else
					{
						FATAL_ERROR(filepath + ":No count field is set for sampler array!");
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
				if (const std::string& stage = stageData.as<std::string>(); stage == "Fragment")
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
				if (const std::string inputRate = inputRateData.as<std::string>(); inputRate == "Per BaseMaterial")
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
		FATAL_ERROR(filepath + ":Failed to load! The file doesn't exist");
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
	}

	YAML::Node materialData = data["Mat"];
	if (!materialData)
	{
		FATAL_ERROR(filepath + ":Base material file doesn't contain identification line or is empty!");
	}

	Material::CreateInfo createInfo;

	if (const auto& baseMaterialData = materialData["Basemat"])
	{
		createInfo.baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(baseMaterialData.as<std::string>());

		if (!createInfo.baseMaterial)
		{
			FATAL_ERROR(baseMaterialData.as<std::string>() + ":There is no such basemat!");
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
			FATAL_ERROR(filepath + ":There is no RenderPass field!");
		}

		Material::RenderPassInfo renderPassInfo{};

		for (const auto & uniformData : pipelineData["Uniforms"])
		{
			std::string uniformName;
			if (const auto& nameData = uniformData["Name"])
			{
				uniformName = nameData.as<std::string>();
			}

			if (UniformLayout::Binding binding = createInfo.baseMaterial->GetPipeline(renderPass)->
				GetChildUniformLayout()->GetBindingByName(uniformName);
				binding.type == UniformLayout::Type::BUFFER)
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

void Serializer::SerializeMaterial(const std::shared_ptr<Material>& material)
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
				void* data = static_cast<char*>(buffer->GetData()) + buffer->GetInstanceSize() * material->GetIndex();

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

	if (const std::string uuid = Utils::FindUuid(material->GetFilepath()); uuid.empty())
	{
		GenerateFileUUID(material->GetFilepath());
	}

	Logger::Log("Material:" + material->GetFilepath() + " has been serialized!", GREEN);
}

void Serializer::SerializeMesh(const std::string& directory,  const std::shared_ptr<Mesh>& mesh)
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
		Utils::GetValue<int>(data, offset) = static_cast<int>(meshName.size());
		offset += sizeof(int);

		memcpy(&Utils::GetValue<char>(data, offset), meshName.data(), meshName.size());
		offset += static_cast<int>(meshName.size());
	}

	// Vertices.
	{
		Utils::GetValue<int>(data, offset) = mesh->GetVertexCount();
		offset += sizeof(int);

		std::vector<float> vertices = mesh->GetRawVertices();
		memcpy(&Utils::GetValue<char>(data, offset), vertices.data(), mesh->GetVertexCount() * sizeof(float));
		offset += mesh->GetVertexCount() * static_cast<int>(sizeof(float));
	}

	// Indices.
	{
		Utils::GetValue<int>(data, offset) = mesh->GetIndexCount();
		offset += sizeof(int);

		// Potential optimization of using 2 bit instead of 4 bit.
		std::vector<uint32_t> indices = mesh->GetRawIndices();
		memcpy(&Utils::GetValue<char>(data, offset), indices.data(), mesh->GetIndexCount() * sizeof(uint32_t));
		offset += mesh->GetIndexCount() * static_cast<int>(sizeof(uint32_t));
	}

	const std::string outMeshFilepath = directory + "/" + meshName + FileFormats::Mesh();
	std::ofstream out(outMeshFilepath, std::ostream::binary);

	out.write(data, static_cast<std::streamsize>(dataSize));

	delete[] data;

	out.close();

	if (const std::string uuid = Utils::FindUuid(mesh->GetFilepath()); uuid.empty())
	{
		GenerateFileUUID(mesh->GetFilepath());
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

	in.seekg(0, std::ifstream::end);
	const int size = in.tellg();
	in.seekg(0, std::ifstream::beg);

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
		offset += verticesSize * static_cast<int>(sizeof(float));
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

void Serializer::SerializeShaderCache(const std::string& filepath, const std::string& code)
{
	const char* directory = "Shaders/Cache/";
	const std::filesystem::path cacheFilepath = directory +
		Utils::EraseDirectoryFromFilePath(filepath) + FileFormats::Spv();
	std::ofstream out(cacheFilepath, std::ostream::binary);

	const size_t lastWriteTime = std::filesystem::last_write_time(filepath).time_since_epoch().count();

	out.write((char*)&lastWriteTime, sizeof(size_t));
	out.write(code.data(), code.size());
	out.close();
}

std::string Serializer::DeserializeShaderCache(const std::string& filepath)
{
	if (filepath.empty())
	{
		return {};
	}

	const char* directory = "Shaders/Cache/";
	const std::filesystem::path cacheFilepath = directory +
		Utils::EraseDirectoryFromFilePath(filepath) + FileFormats::Spv();
	if (std::filesystem::exists(cacheFilepath))
	{
		std::ifstream in(cacheFilepath, std::ifstream::binary);

		in.seekg(0, std::ifstream::end);
		auto size = in.tellg();
		size -= sizeof(size_t);
		in.seekg(0, std::ifstream::beg);

		std::string data;
		data.resize(size);

		size_t lastWriteTime = 0;
		in.read((char*)&lastWriteTime, sizeof(size_t));

		if(lastWriteTime != std::filesystem::last_write_time(filepath).time_since_epoch().count())
		{
			return {};
		}

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
	constexpr auto importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices |
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

	// Consider that only a hierarchy of game objects can be serilized as a prefab.
	if (std::shared_ptr<Entity> root = GenerateEntity(scene->mRootNode, meshesByIndex, materialsByMeshes);
		root->GetChilds().size() > 0)
	{
		SerializePrefab(directory + "/" + root->GetName() + FileFormats::Prefab(), root);
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
	std::string meshFilepath = directory + "/" + meshName + FileFormats::Mesh();

	int containIndex = 0;
	while (MeshManager::GetInstance().GetMesh(meshFilepath))
	{
		meshName = defaultMeshName + "_" + std::to_string(containIndex);
		meshFilepath = directory + "/" + meshName + FileFormats::Mesh();
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

	constexpr size_t vertexOffset = sizeof(Vertex) / sizeof(float);
	constexpr size_t tangentOffset = offsetof(Vertex, tangent) / sizeof(float);
	constexpr size_t bitangentOffset = offsetof(Vertex, bitangent) / sizeof(float);
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
	SerializeMesh(Utils::ExtractDirectoryFromFilePath(mesh->GetFilepath()), mesh);

	return mesh;
}

std::shared_ptr<Material> Serializer::GenerateMaterial(const aiMaterial* aiMaterial, const std::string& directory)
{
	aiString aiMaterialName;
	aiMaterial->Get(AI_MATKEY_NAME, aiMaterialName);

	const std::string materialName = std::string(aiMaterialName.C_Str());
	if (materialName == "DefaultMaterial")
	{
		return MaterialManager::GetInstance().LoadMaterial("Materials/MeshBase.mat");
	}

	std::string materialFilepath = directory + "/" + materialName + ".mat";

	const std::shared_ptr<Material> meshBaseMaterial = MaterialManager::GetInstance().LoadMaterial("Materials/MeshBase.mat");
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

		constexpr float useNormalMap = 1.0f;
		glm::vec4 other = { useNormalMap, 0.0f, 0.0f, 0.0f };
		material->SetValue("material", "other", other);
	}

	Material::Save(material);

	return material;
}

std::shared_ptr<Entity> Serializer::GenerateEntity(const aiNode* assimpNode,
	const std::unordered_map<size_t, std::shared_ptr<Mesh>>& meshesByIndex,
	const std::unordered_map<std::shared_ptr<Mesh>, std::shared_ptr<Material>>& materialsByMeshes)
{
	if (!assimpNode)
	{
		return nullptr;
	}

	const std::shared_ptr<Scene> scene = SceneManager::GetInstance().GetSceneByTag("GenerateGameObject");
	std::shared_ptr<Entity> node = scene->CreateEntity(assimpNode->mName.C_Str());
	Transform& nodeTransform = node->AddComponent<Transform>(node);

	Renderer3D* r3d = nullptr;
	if (assimpNode->mNumMeshes > 0)
	{
		r3d = &node->AddComponent<Renderer3D>();
	}

	for (size_t meshIndex = 0; meshIndex < assimpNode->mNumMeshes; meshIndex++)
	{
		if (const auto meshByIndex = meshesByIndex.find(assimpNode->mMeshes[meshIndex]); meshByIndex != meshesByIndex.end())
		{
			r3d->mesh = meshByIndex->second;
			r3d->material = materialsByMeshes.find(meshByIndex->second)->second;
		}
	}

	for (size_t childIndex = 0; childIndex < assimpNode->mNumChildren; childIndex++)
	{
		const aiNode* child = assimpNode->mChildren[childIndex];
		node->AddChild(GenerateEntity(child, meshesByIndex, materialsByMeshes));
	}

	aiVector3D aiPosition;
	aiVector3D aiRotation;
	aiVector3D aiScale;
	assimpNode->mTransformation.Decompose(aiScale, aiRotation, aiPosition);

	const glm::vec3 position = { aiPosition.x, aiPosition.y, aiPosition.z };
	const glm::vec3 rotation = { aiRotation.x, aiRotation.y, aiRotation.z };
	const glm::vec3 scale = { aiScale.x, aiScale.y, aiScale.z };

	nodeTransform.Translate(position);
	nodeTransform.Rotate(rotation);
	nodeTransform.Scale(scale);

	return node;
}

void Serializer::SerializeEntity(YAML::Emitter& out, const std::shared_ptr<Entity>& entity, const bool withChilds)
{
	if (!entity)
	{
		return;
	}

	out << YAML::BeginMap;

	out << YAML::Key << "UUID" << YAML::Value << entity->GetUUID();
	out << YAML::Key << "Name" << YAML::Value << entity->GetName();
	out << YAML::Key << "IsEnabled" << YAML::Value << entity->IsEnabled();

	std::vector<std::string> childUUIDs;
	for (const std::shared_ptr<Entity>& child : entity->GetChilds())
	{
		childUUIDs.emplace_back(child->GetUUID());
	}

	out << YAML::Key << "Childs" << YAML::Value << childUUIDs;

	SerializeTransform(out, entity);
	SerializeCamera(out, entity);
	SerializeRenderer3D(out, entity);
	SerializePointLight(out, entity);

	out << YAML::EndMap;

	if (!withChilds)
	{
		return;
	}

	for (const std::shared_ptr<Entity>& child : entity->GetChilds())
	{
		SerializeEntity(out, child);
	}
}

std::shared_ptr<Entity> Serializer::DeserializeEntity(
	const YAML::Node& in,
	const std::shared_ptr<Scene>& scene,
	std::vector<std::string>& childs)
{
	UUID uuid;
	if (const auto& uuidData = in["UUID"])
	{
		uuid = uuidData.as<std::string>();
	}

	std::string name;
	if (const auto& nameData = in["Name"])
	{
		name = nameData.as<std::string>();
	}

	std::shared_ptr<Entity> entity = scene->CreateEntity(name, uuid);

	if (const auto& isEnabledData = in["IsEnabled"])
	{
		entity->SetEnabled(isEnabledData.as<bool>());
	}

	for (const auto& childsData : in["Childs"])
	{
		childs.emplace_back(childsData.as<std::string>());
	}

	DeserializeTransform(in, entity);
	DeserializeCamera(in, entity);
	DeserializeRenderer3D(in, entity);
	DeserializePointLight(in, entity);

	return entity;
}

void Serializer::SerializePrefab(const std::string& filepath, const std::shared_ptr<Entity>& entity)
{
	YAML::Emitter out;

	out << YAML::BeginSeq;

	SerializeEntity(out, entity);

	out << YAML::EndSeq;

	std::ofstream fout(filepath);
	fout << out.c_str();
	fout.close();
}

void Serializer::DeserializePrefab(const std::string& filepath, const std::shared_ptr<Scene>& scene)
{
	if (filepath.empty() || filepath == none || Utils::GetFileFormat(filepath) != FileFormats::Prefab())
	{
		Logger::Error(filepath + ":Failed to load prefab! Filepath is incorrect!");
		return;
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
	}

	std::unordered_map<std::shared_ptr<Entity>, std::vector<std::string>> childsUUIDByEntity;
	for (const auto& entitiyData : data)
	{
		std::vector<std::string> childs;
		std::shared_ptr<Entity> entity = DeserializeEntity(entitiyData, scene, childs);
		childsUUIDByEntity[entity] = std::move(childs);
	}

	for (const auto& [entity, childsUUID] : childsUUIDByEntity)
	{
		for (const auto& childUUID : childsUUID)
		{
			if (std::shared_ptr<Entity> child = scene->FindEntityByUUID(childUUID))
			{
				entity->AddChild(child);
			}
			else
			{
				FATAL_ERROR("Child entity is not valid!");
			}
		}
	}

	Logger::Log("Prefab:" + filepath + " has been deserialized!", GREEN);
}

void Serializer::SerializeTransform(YAML::Emitter& out, const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<Transform>())
	{
		return;
	}

	const Transform& transform = entity->GetComponent<Transform>();

	out << YAML::Key << "Transform";

	out << YAML::BeginMap;

	out << YAML::Key << "Position" << YAML::Value << transform.GetPosition();
	out << YAML::Key << "Rotation" << YAML::Value << transform.GetRotation();
	out << YAML::Key << "Scale" << YAML::Value << transform.GetScale();
	out << YAML::Key << "FollowOwner" << YAML::Value << transform.GetFollorOwner();

	out << YAML::EndMap;
}

void Serializer::DeserializeTransform(const YAML::Node& in, const std::shared_ptr<Entity>& entity)
{
	if (const auto& transformData = in["Transform"])
	{
		if (!entity->HasComponent<Transform>())
		{
			entity->AddComponent<Transform>(entity);
		}

		Transform& transform = entity->GetComponent<Transform>();

		if (const auto& positionData = transformData["Position"])
		{
			transform.Translate(positionData.as<glm::vec3>());
		}

		if (const auto& rotationData = transformData["Rotation"])
		{
			transform.Rotate(rotationData.as<glm::vec3>());
		}

		if (const auto& scaleData = transformData["Scale"])
		{
			transform.Scale(scaleData.as<glm::vec3>());
		}

		if (const auto& follorOwnerData = transformData["FollowOwner"])
		{
			transform.SetFollowOwner(follorOwnerData.as<bool>());
		}
	}
}

void Serializer::SerializeRenderer3D(YAML::Emitter& out, const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<Renderer3D>())
	{
		return;
	}

	const Renderer3D& r3d = entity->GetComponent<Renderer3D>();

	out << YAML::Key << "Renderer3D";

	out << YAML::BeginMap;

	out << YAML::Key << "Mesh" << YAML::Value << Utils::FindUuid(r3d.mesh->GetFilepath());
	out << YAML::Key << "Material" << YAML::Value << Utils::FindUuid(r3d.material->GetFilepath());

	out << YAML::EndMap;
}

void Serializer::DeserializeRenderer3D(const YAML::Node& in, const std::shared_ptr<Entity>& entity)
{
	if (const auto& renderer3DData = in["Renderer3D"])
	{
		if (!entity->HasComponent<Renderer3D>())
		{
			entity->AddComponent<Renderer3D>();
		}

		Renderer3D& r3d = entity->GetComponent<Renderer3D>();

		if (const auto& meshData = renderer3DData["Mesh"])
		{
			const std::string uuid = meshData.as<std::string>();
			const std::shared_ptr<Mesh> mesh = MeshManager::GetInstance().LoadMesh(
				Utils::GetShortFilepath(Utils::Find(uuid, filepathByUuid)));
			r3d.mesh = mesh;
		}

		if (const auto& materialData = renderer3DData["Material"])
		{
			const std::string uuid = materialData.as<std::string>();
			const std::shared_ptr<Material> material = MaterialManager::GetInstance().LoadMaterial(
				Utils::GetShortFilepath(Utils::Find(uuid, filepathByUuid)));
			r3d.material = material;
		}
	}
}

void Serializer::SerializePointLight(YAML::Emitter& out, const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<PointLight>())
	{
		return;
	}

	const PointLight& pointLight = entity->GetComponent<PointLight>();

	out << YAML::Key << "PointLight";

	out << YAML::BeginMap;

	out << YAML::Key << "Color" << YAML::Value << pointLight.color;
	out << YAML::Key << "Constant" << YAML::Value << pointLight.constant;
	out << YAML::Key << "Quadratic" << YAML::Value << pointLight.quadratic;
	out << YAML::Key << "Linear" << YAML::Value << pointLight.linear;

	out << YAML::EndMap;
}

void Serializer::DeserializePointLight(const YAML::Node& in, const std::shared_ptr<Entity>& entity)
{
	if (const auto& pointLightData = in["PointLight"])
	{
		if (!entity->HasComponent<PointLight>())
		{
			entity->AddComponent<PointLight>();
		}

		PointLight& pointLight = entity->GetComponent<PointLight>();

		if (const auto& colorData = pointLightData["Color"])
		{
			pointLight.color = colorData.as<glm::vec3>();
		}

		if (const auto& constantData = pointLightData["Constant"])
		{
			pointLight.constant = constantData.as<float>();
		}

		if (const auto& quadraticData = pointLightData["Quadratic"])
		{
			pointLight.quadratic = quadraticData.as<float>();
		}

		if (const auto& linearData = pointLightData["Linear"])
		{
			pointLight.linear = linearData.as<float>();
		}
	}
}

void Serializer::SerializeCamera(YAML::Emitter& out, const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<Camera>())
	{
		return;
	}

	const Camera& camera = entity->GetComponent<Camera>();

	out << YAML::Key << "Camera";

	out << YAML::BeginMap;

	out << YAML::Key << "Fov" << YAML::Value << camera.GetFov();
	out << YAML::Key << "Type" << YAML::Value << static_cast<int>(camera.GetType());
	out << YAML::Key << "ZNear" << YAML::Value << camera.GetZNear();
	out << YAML::Key << "ZFar" << YAML::Value << camera.GetZFar();
	out << YAML::Key << "Viewport" << YAML::Value << camera.GetViewport();

	out << YAML::EndMap;
}

void Serializer::DeserializeCamera(const YAML::Node& in, const std::shared_ptr<Entity>& entity)
{
	if (const auto& cameraData = in["Camera"])
	{
		if (!entity->HasComponent<Camera>())
		{
			entity->AddComponent<Camera>(entity);
		}

		Camera& camera = entity->GetComponent<Camera>();

		if (const auto& fovData = cameraData["Fov"])
		{
			camera.SetFov(fovData.as<float>());
		}

		if (const auto& typeData = cameraData["Type"])
		{
			camera.SetType(static_cast<Camera::Type>(typeData.as<int>()));
		}

		if (const auto& zNearData = cameraData["ZNear"])
		{
			camera.SetZNear(zNearData.as<float>());
		}

		if (const auto& zFarData = cameraData["ZFar"])
		{
			camera.SetZFar(zFarData.as<float>());
		}

		if (const auto& viewportData = cameraData["Viewport"])
		{
			if (const std::shared_ptr<Viewport> viewport = ViewportManager::GetInstance().GetViewport(viewportData.as<std::string>()))
			{
				viewport->SetCamera(entity);
			}
		}
	}
}

void Serializer::SerializeScene(const std::string& filepath, const std::shared_ptr<Scene>& scene)
{
	if (!scene)
	{
		Logger::Error("Failed to serialize a scene, the scene is invalid!");
		return;
	}

	YAML::Emitter out;

	out << YAML::BeginSeq;

	for (const auto& entity : scene->GetEntities())
	{
		SerializeEntity(out, entity, false);
	}

	out << YAML::EndSeq;

	std::ofstream fout(filepath);
	fout << out.c_str();
	fout.close();

	Logger::Log("Scene:" + filepath + " has been serialized!", GREEN);
}

std::shared_ptr<Scene> Serializer::DeserializeScene(const std::string& filepath)
{
	if (filepath.empty() || filepath == none || Utils::GetFileFormat(filepath) != FileFormats::Scene())
	{
		Logger::Error(filepath + ":Failed to load scene! Filepath is incorrect!");
		return nullptr;
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
	}

	std::shared_ptr<Scene> scene = SceneManager::GetInstance().Create(
		Utils::EraseFileFormat(Utils::EraseDirectoryFromFilePath(filepath)),
		"Main");
	scene->SetFilepath(filepath);

	std::unordered_map<std::shared_ptr<Entity>, std::vector<std::string>> childsUUIDByEntity;
	for (const auto& entityData : data)
	{
		std::vector<std::string> childs;
		std::shared_ptr<Entity> entity = DeserializeEntity(entityData, scene, childs);
		childsUUIDByEntity[entity] = std::move(childs);
	}

	for (const auto& [entity, childsUUID] : childsUUIDByEntity)
	{
		for (const auto& childUUID : childsUUID)
		{
			if (std::shared_ptr<Entity> child = scene->FindEntityByUUID(childUUID))
			{
				entity->AddChild(child);
			}
			else
			{
				FATAL_ERROR("Child entity is not valid!");
			}
		}
	}

	Logger::Log("Scene:" + filepath + " has been deserialized!", GREEN);

	return scene;
}

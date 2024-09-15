#include "Serializer.h"

#include "FileFormatNames.h"
#include "Logger.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "RenderPassManager.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include "ViewportManager.h"
#include "Viewport.h"

#include "../Components/Camera.h"
#include "../Components/DirectionalLight.h"
#include "../Components/PointLight.h"
#include "../Components/Renderer3D.h"
#include "../Components/Transform.h"

#include "../Utils/Utils.h"

#include "../Graphics/Vertex.h"

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

YAML::Emitter& operator<<(YAML::Emitter& out, const std::filesystem::path &filepath)
{
	out << filepath.string();
	return out;
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

EngineConfig Serializer::DeserializeEngineConfig(const std::filesystem::path& filepath)
{
	if (filepath.empty() || filepath == none)
	{
		FATAL_ERROR(filepath.string() + ":Failed to load engine config! Filepath is incorrect!");
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath.string() + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
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
			const std::filesystem::path filepath = entry.path();
			std::filesystem::path metaFilepath = filepath;
			metaFilepath.concat(FileFormats::Meta());

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
						FATAL_ERROR(filepath.string() + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
					}

					UUID uuid;
					if (YAML::Node uuidData = data["UUID"]; !uuidData)
					{
						FATAL_ERROR(filepath.string() + ":Failed to load meta file!");
					}
					else
					{
						uuid = uuidData.as<std::string>();
					}

					const std::filesystem::path shortFilepath = Utils::GetShortFilepath(filepath);
					filepathByUuid[uuid] = shortFilepath;
					uuidByFilepath[shortFilepath] = uuid;
				}
			}
		}
	}
}

std::string Serializer::GenerateFileUUID(const std::filesystem::path& filepath)
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

	std::filesystem::path metaFilepath = filepath;
	metaFilepath.concat(".meta"); 
	std::ofstream fout(metaFilepath);
	fout << out.c_str();
	fout.close();

	filepathByUuid[uuid] = filepath;
	uuidByFilepath[filepath] = uuid;

	return uuid;
}

BaseMaterial::CreateInfo Serializer::LoadBaseMaterial(const std::filesystem::path& filepath)
{
	if (!std::filesystem::exists(filepath))
	{
		FATAL_ERROR(filepath.string() + ":Failed to load! The file doesn't exist");
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath.string() + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
	}

	YAML::Node materialData = data["Basemat"];
	if (!materialData)
	{
		FATAL_ERROR(filepath.string() + ":Base material file doesn't contain identification line or is empty!");
	}

	BaseMaterial::CreateInfo createInfo{};

	std::vector<Pipeline::CreateInfo>& pipelineCreateInfos = createInfo.pipelineCreateInfos;
	for (const auto& pipelineData : materialData["Pipelines"])
	{
		std::string renderPassName;

		Pipeline::CreateInfo pipelineCreateInfo{};
		if (const auto& renderPassData = pipelineData["RenderPass"])
		{
			renderPassName = renderPassData.as<std::string>();
			pipelineCreateInfo.renderPass = RenderPassManager::GetInstance().GetRenderPass(renderPassName);
		}

		if (const auto& depthTestData = pipelineData["DepthTest"])
		{
			pipelineCreateInfo.depthTest = depthTestData.as<bool>();
		}

		if (const auto& depthWriteData = pipelineData["DepthWrite"])
		{
			pipelineCreateInfo.depthWrite = depthWriteData.as<bool>();
		}

		if (const auto& depthCompareData = pipelineData["DepthCompare"])
		{
			const std::string depthCompare = depthCompareData.as<std::string>();
			if (depthCompare == "Never")
			{
				pipelineCreateInfo.depthCompare = Pipeline::DepthCompare::NEVER;
			}
			else if (depthCompare == "Less")
			{
				pipelineCreateInfo.depthCompare = Pipeline::DepthCompare::LESS;
			}
			else if (depthCompare == "Equal")
			{
				pipelineCreateInfo.depthCompare = Pipeline::DepthCompare::EQUAL;
			}
			else if (depthCompare == "LessOrEqual")
			{
				pipelineCreateInfo.depthCompare = Pipeline::DepthCompare::LESS_OR_EQUAL;
			}
			else if (depthCompare == "Greater")
			{
				pipelineCreateInfo.depthCompare = Pipeline::DepthCompare::GREATER;
			}
			else if (depthCompare == "NotEqual")
			{
				pipelineCreateInfo.depthCompare = Pipeline::DepthCompare::NOT_EQUAL;
			}
			else if (depthCompare == "GreaterOrEqual")
			{
				pipelineCreateInfo.depthCompare = Pipeline::DepthCompare::GREATER_OR_EQUAL;
			}
			else if (depthCompare == "Always")
			{
				pipelineCreateInfo.depthCompare = Pipeline::DepthCompare::ALWAYS;
			}
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

		if (const auto& topologyModeData = pipelineData["TopologyMode"])
		{
			if (const std::string& topologyMode = topologyModeData.as<std::string>(); topologyMode == "LineList")
			{
				pipelineCreateInfo.topologyMode = Pipeline::TopologyMode::LINE_LIST;
			}
			else if (topologyMode == "PointList")
			{
				pipelineCreateInfo.topologyMode = Pipeline::TopologyMode::POINT_LIST;
			}
			else if (topologyMode == "TriangleList")
			{
				pipelineCreateInfo.topologyMode = Pipeline::TopologyMode::TRIANGLE_LIST;
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
			pipelineCreateInfo.shaderFilepathsByType[Pipeline::ShaderType::VERTEX] = vertexData.as<std::string>();
		}

		if (const auto& fragmentData = pipelineData["Fragment"])
		{
			pipelineCreateInfo.shaderFilepathsByType[Pipeline::ShaderType::FRAGMENT] = fragmentData.as<std::string>();
		}

		if (const auto& geometryData = pipelineData["Geometry"])
		{
			pipelineCreateInfo.shaderFilepathsByType[Pipeline::ShaderType::GEOMETRY] = geometryData.as<std::string>();
		}

		if (const auto& computeData = pipelineData["Compute"])
		{
			pipelineCreateInfo.shaderFilepathsByType[Pipeline::ShaderType::COMPUTE] = computeData.as<std::string>();
		}

		for (const auto& descriptorSetData : pipelineData["DescriptorSets"])
		{
			Pipeline::DescriptorSetIndexType type{};
			if (const auto& typeData = descriptorSetData["Type"])
			{
				std::string typeName = typeData.as<std::string>();
				if (typeName == "Renderer")
				{
					type = Pipeline::DescriptorSetIndexType::RENDERER;
				}
				else if (typeName == "RenderPass")
				{
					type = Pipeline::DescriptorSetIndexType::RENDERPASS;
				}
				else if (typeName == "BaseMaterial")
				{
					type = Pipeline::DescriptorSetIndexType::BASE_MATERIAL;
				}
				else if (typeName == "Material")
				{
					type = Pipeline::DescriptorSetIndexType::MATERIAL;
				}
			}

			std::string attachedrenderPassName = renderPassName;
			if (const auto& renderPassNameData = descriptorSetData["RenderPass"])
			{
				attachedrenderPassName = renderPassNameData.as<std::string>();
			}

			uint32_t set = 0;
			if (const auto& setData = descriptorSetData["Set"])
			{
				set = setData.as<uint32_t>();
			}

			pipelineCreateInfo.descriptorSetIndicesByType[type][attachedrenderPassName] = set;
		}

		for (const auto& vertexInputBindingDescriptionData : pipelineData["VertexInputBindingDescriptions"])
		{
			auto& bindingDescription = pipelineCreateInfo.bindingDescriptions.emplace_back();

			if (const auto& bindingData = vertexInputBindingDescriptionData["Binding"])
			{
				bindingDescription.binding = bindingData.as<uint32_t>();
			}

			if (const auto& inputRateData = vertexInputBindingDescriptionData["InputRate"])
			{
				std::string inputRateName = inputRateData.as<std::string>();
				if (inputRateName == "Vertex")
				{
					bindingDescription.inputRate = Pipeline::InputRate::VERTEX;
				}
				else if (inputRateName == "Instance")
				{
					bindingDescription.inputRate = Pipeline::InputRate::INSTANCE;
				}
			}

			for (const auto& nameData : vertexInputBindingDescriptionData["Names"])
			{
				bindingDescription.names.emplace_back(nameData.as<std::string>());
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

		ParseUniformValues(pipelineData, pipelineCreateInfo.uniformInfo);

		pipelineCreateInfos.emplace_back(pipelineCreateInfo);
	}

	Logger::Log("BaseMaterial:" + filepath.string() + " has been deserialized!", GREEN);

	return createInfo;
}

Material::CreateInfo Serializer::LoadMaterial(const std::filesystem::path& filepath)
{
	if (!std::filesystem::exists(filepath))
	{
		FATAL_ERROR(filepath.string() + ":Failed to load! The file doesn't exist");
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath.string() + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
	}

	YAML::Node materialData = data["Mat"];
	if (!materialData)
	{
		FATAL_ERROR(filepath.string() + ":Base material file doesn't contain identification line or is empty!");
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

	std::unordered_map<std::string, Material::Option>& optionsByName = createInfo.optionsByName;
	for (const auto& optionData : materialData["Options"])
	{
		std::string name;
		if (const auto& nameData = optionData["Name"])
		{
			name = nameData.as<std::string>();
		}

		if (name.empty())
		{
			Logger::Warning(filepath.string() + ": Option name is empty, option will be ignored!");
			continue;
		}

		Material::Option option{};
		if (const auto& isEnabledData = optionData["IsEnabled"])
		{
			option.m_IsEnabled = isEnabledData.as<bool>();
		}

		for (const auto& activeData : optionData["Active"])
		{
			option.m_Active.emplace_back(activeData.as<std::string>());
		}

		for (const auto& activeData : optionData["Inactive"])
		{
			option.m_Inactive.emplace_back(activeData.as<std::string>());
		}

		optionsByName.emplace(name, option);
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
			FATAL_ERROR(filepath.string() + ":There is no RenderPass field!");
		}

		Pipeline::UniformInfo uniformInfo{};
		ParseUniformValues(pipelineData, uniformInfo);

		createInfo.uniformInfos.emplace(renderPass, uniformInfo);
	}

	Logger::Log("Material:" + filepath.string() + " has been deserialized!", GREEN);

	return createInfo;
}

void Serializer::SerializeMaterial(const std::shared_ptr<Material>& material)
{
	YAML::Emitter out;

	out << YAML::BeginMap;

	out << YAML::Key << "Mat";

	out << YAML::BeginMap;

	out << YAML::Key << "Basemat" << YAML::Value << material->GetBaseMaterial()->GetFilepath();

	out << YAML::Key << "Options";

	out << YAML::BeginSeq;

	for (const auto& [name, option] : material->GetOptionsByName())
	{
		out << YAML::BeginMap;

		out << YAML::Key << "Name" << YAML::Value << name;
		out << YAML::Key << "IsEnabled" << YAML::Value << option.m_IsEnabled;

		out << YAML::Key << "Active";

		out << YAML::BeginSeq;

		for (const auto& active : option.m_Active)
		{
			out << active;
		}

		out << YAML::EndSeq;

		out << YAML::Key << "Inactive";

		out << YAML::BeginSeq;

		for (const auto& inactive : option.m_Inactive)
		{
			out << inactive;
		}

		out << YAML::EndSeq;

		out << YAML::EndMap;
	}

	out << YAML::EndSeq;

	out << YAML::Key << "Pipelines";

	out << YAML::BeginSeq;

	for (const auto& [renderPassName, pipeline] : material->GetBaseMaterial()->GetPipelinesByRenderPass())
	{
		out << YAML::BeginMap;

		out << YAML::Key << "RenderPass" << YAML::Value << renderPassName;

		out << YAML::Key << "Uniforms";

		out << YAML::BeginSeq;

		std::optional<uint32_t> descriptorSetIndex = pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::MATERIAL, renderPassName);
		if (!descriptorSetIndex)
		{
			continue;
		}
		for (const auto& binding : pipeline->GetUniformLayout(*descriptorSetIndex)->GetBindings())
		{
			out << YAML::BeginMap;

			out << YAML::Key << "Name" << YAML::Value << binding.name;

			if (binding.type == ShaderReflection::Type::COMBINED_IMAGE_SAMPLER)
			{
				out << YAML::Key << "Value" << YAML::Value << 
				Utils::FindUuid(material->GetUniformWriter(renderPassName)->GetTexture(binding.name)->GetFilepath());
			}
			else if (binding.type == ShaderReflection::Type::UNIFORM_BUFFER)
			{
				std::shared_ptr<Buffer> buffer = material->GetBuffer(binding.name);
				void* data = buffer->GetData();

				out << YAML::Key << "Values";

				out << YAML::BeginSeq;

				std::function<void(const ShaderReflection::ReflectVariable&, std::string)> saveValue = [&saveValue, &out, &data]
				(const ShaderReflection::ReflectVariable& value, std::string parentName)
				{
					parentName += value.name;

					if (value.type == ShaderReflection::ReflectVariable::Type::STRUCT)
					{
						for (const auto& memberValue : value.variables)
						{
							saveValue(memberValue, parentName + ".");
						}
						return;
					}

					out << YAML::BeginMap;

					out << YAML::Key << "Name" << YAML::Value << parentName;

					if (value.type == ShaderReflection::ReflectVariable::Type::VEC2)
					{
						out << YAML::Key << "Value" << YAML::Value << Utils::GetValue<glm::vec2>(data, value.offset);
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::VEC3)
					{
						out << YAML::Key << "Value" << YAML::Value << Utils::GetValue<glm::vec3>(data, value.offset);
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::VEC4)
					{
						out << YAML::Key << "Value" << YAML::Value << Utils::GetValue<glm::vec4>(data, value.offset);
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::FLOAT)
					{
						out << YAML::Key << "Value" << YAML::Value << Utils::GetValue<float>(data, value.offset);
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::INT)
					{
						out << YAML::Key << "Value" << YAML::Value << Utils::GetValue<int>(data, value.offset);
					}

					out << YAML::Key << "Type" << ShaderReflection::ConvertTypeToString(value.type);

					out << YAML::EndMap;
				};
				for (const auto& value : binding.buffer->variables)
				{
					saveValue(value, "");
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

	Logger::Log("Material:" + material->GetFilepath().string() + " has been serialized!", GREEN);
}

void Serializer::SerializeMesh(const std::filesystem::path& directory,  const std::shared_ptr<Mesh>& mesh)
{
	const std::string meshName = mesh->GetName();

	size_t dataSize = mesh->GetVertexCount() * sizeof(Vertex) +
		mesh->GetIndexCount() * sizeof(uint32_t) +
		mesh->GetName().size() +
		mesh->GetFilepath().string().size() +
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
		memcpy(&Utils::GetValue<char>(data, offset), vertices.data(), mesh->GetVertexCount() * sizeof(Vertex));
		offset += mesh->GetVertexCount() * static_cast<int>(sizeof(Vertex));
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

	std::filesystem::path outMeshFilepath = directory / (meshName + FileFormats::Mesh());
	std::ofstream out(outMeshFilepath, std::ostream::binary);

	out.write(data, static_cast<std::streamsize>(dataSize));

	delete[] data;

	out.close();

	if (const std::string uuid = Utils::FindUuid(mesh->GetFilepath()); uuid.empty())
	{
		GenerateFileUUID(mesh->GetFilepath());
	}

	Logger::Log("Mesh:" + outMeshFilepath.string() + " has been serialized!", GREEN);
}

std::shared_ptr<Mesh> Serializer::DeserializeMesh(const std::filesystem::path& filepath)
{
	if (!std::filesystem::exists(filepath))
	{
		Logger::Error(filepath.string() + ": doesn't exist!");
		return nullptr;
	}

	if (FileFormats::Mesh() != Utils::GetFileFormat(filepath))
	{
		Logger::Error(filepath.string() + ": is not mesh asset!");
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

		vertices.resize(verticesSize * (sizeof(Vertex) / sizeof(float)));
		memcpy(vertices.data(), &Utils::GetValue<char>(data, offset), verticesSize * sizeof(Vertex));
		offset += verticesSize * static_cast<int>(sizeof(Vertex));
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

	Logger::Log("Mesh:" + filepath.string() + " has been deserialized!", GREEN);

	return std::make_shared<Mesh>(meshName, filepath, vertices, indices);
}

void Serializer::SerializeShaderCache(const std::filesystem::path& filepath, const std::string& code)
{
	const std::filesystem::path directory = "Shaders/Cache/";

	if (!std::filesystem::exists(directory))
	{
		std::filesystem::create_directories(directory);
	}

	std::filesystem::path cacheFilepath = directory / filepath.filename();
	cacheFilepath.concat(FileFormats::Spv());
	std::ofstream out(cacheFilepath, std::ostream::binary);

	const size_t lastWriteTime = std::filesystem::last_write_time(filepath).time_since_epoch().count();

	out.write((char*)&lastWriteTime, sizeof(size_t));
	out.write(code.data(), code.size());
	out.close();
}

std::string Serializer::DeserializeShaderCache(const std::filesystem::path& filepath)
{
	if (filepath.empty())
	{
		return {};
	}

	const std::filesystem::path directory = "Shaders/Cache/";
	std::filesystem::path cacheFilepath = directory / filepath.filename();
	cacheFilepath.concat(FileFormats::Spv());
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
Serializer::LoadIntermediate(const std::filesystem::path& filepath)
{
	const std::filesystem::path directory = filepath.parent_path();

	Assimp::Importer import;
	constexpr auto importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices |
		aiProcess_OptimizeMeshes | aiProcess_RemoveRedundantMaterials | aiProcess_ImproveCacheLocality |
		aiProcess_JoinIdenticalVertices | aiProcess_GenUVCoords | aiProcess_SortByPType | aiProcess_FindInstances |
		aiProcess_ValidateDataStructure | aiProcess_FindDegenerates | aiProcess_FindInvalidData;
	const aiScene* scene = import.ReadFile(filepath.string(), importFlags);

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
		std::filesystem::path prefabFilepath = directory / root->GetName();
		prefabFilepath.replace_extension(FileFormats::Prefab());
		SerializePrefab(prefabFilepath, root);
	}

	SceneManager::GetInstance().Delete("GenerateGameObject");

	return {};
}

std::shared_ptr<Mesh> Serializer::GenerateMesh(aiMesh* aiMesh, const std::filesystem::path& directory)
{
	std::vector<float> vertices;
	std::vector<uint32_t> indices;

	std::string defaultMeshName = aiMesh->mName.C_Str();
	std::string meshName = defaultMeshName;
	std::filesystem::path meshFilepath = directory / (meshName + FileFormats::Mesh());

	int containIndex = 0;
	while (MeshManager::GetInstance().GetMesh(meshFilepath))
	{
		meshName = defaultMeshName + "_" + std::to_string(containIndex);
		meshFilepath.replace_filename(meshName);
		containIndex++;
	}

	for (size_t vertexIndex = 0; vertexIndex < aiMesh->mNumVertices; vertexIndex++)
	{
		aiVector3D vertex = aiMesh->mVertices[vertexIndex];
		vertices.emplace_back(vertex.x);
		vertices.emplace_back(vertex.y);
		vertices.emplace_back(vertex.z);

		if (aiMesh->HasTextureCoords(0))
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

		if (aiMesh->HasTextureCoords(0) && !aiMesh->HasTangentsAndBitangents())
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
	SerializeMesh(mesh->GetFilepath().parent_path(), mesh);

	return mesh;
}

std::shared_ptr<Material> Serializer::GenerateMaterial(const aiMaterial* aiMaterial, const std::filesystem::path& directory)
{
	aiString aiMaterialName;
	aiMaterial->Get(AI_MATKEY_NAME, aiMaterialName);

	const std::string materialName = std::string(aiMaterialName.C_Str());
	if (materialName == "DefaultMaterial")
	{
		return MaterialManager::GetInstance().LoadMaterial("Materials/MeshBase.mat");
	}

	std::filesystem::path materialFilepath = directory / materialName;
	materialFilepath.concat(FileFormats::Mat());

	const std::shared_ptr<Material> meshBaseMaterial = MaterialManager::GetInstance().LoadMaterial("Materials/MeshBase.mat");
	const std::shared_ptr<Material> material = MaterialManager::GetInstance().Clone(materialName, materialFilepath, meshBaseMaterial);
	const std::shared_ptr<UniformWriter> uniformWriter = material->GetUniformWriter(GBuffer);
	
	aiColor3D aiColor(0.0f, 0.0f, 0.0f);
	if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == aiReturn_SUCCESS)
	{
		glm::vec4 color = { aiColor.r, aiColor.g, aiColor.b, 1.0f };
		material->WriteToBuffer("GBufferMaterial", "material.color", color);
	}

	// Note: Not sure how to do it correctly.
	/*float metallic = 0.0f;
	if (aiMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS)
	{
		material->WriteToBuffer("material", "metallic", metallic);
	}
	
	float roughness = 0.0f;
	if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
	{
		material->WriteToBuffer("material", "roughness", metallic);
	}*/

	uint32_t numTextures = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
	aiString aiTextureName;
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("albedoTexture", TextureManager::GetInstance().Load(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_NORMALS);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("normalTexture", TextureManager::GetInstance().Load(textureFilepath));

		constexpr int useNormalMap = 1;
		material->WriteToBuffer("GBufferMaterial", "material.useNormalMap", useNormalMap);
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_METALNESS);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("metalnessTexture", TextureManager::GetInstance().Load(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("roughnessTexture", TextureManager::GetInstance().Load(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_AMBIENT_OCCLUSION, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("aoTexture", TextureManager::GetInstance().Load(textureFilepath));
	}

	material->GetBuffer("GBufferMaterial")->Flush();
	uniformWriter->Flush();

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
	SerializeDirectionalLight(out, entity);

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
	DeserializeDirectionalLight(in, entity);

	return entity;
}

void Serializer::SerializePrefab(const std::filesystem::path& filepath, const std::shared_ptr<Entity>& entity)
{
	YAML::Emitter out;

	out << YAML::BeginSeq;

	SerializeEntity(out, entity);

	out << YAML::EndSeq;

	std::ofstream fout(filepath);
	fout << out.c_str();
	fout.close();
}

void Serializer::DeserializePrefab(const std::filesystem::path& filepath, const std::shared_ptr<Scene>& scene)
{
	if (filepath.empty() || filepath == none || Utils::GetFileFormat(filepath) != FileFormats::Prefab())
	{
		Logger::Error(filepath.string() + ":Failed to load prefab! Filepath is incorrect!");
		return;
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath.string() + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
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

	Logger::Log("Prefab:" + filepath.string() + " has been deserialized!", GREEN);
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

	if (Utils::FindUuid(r3d.material->GetFilepath()).empty())
	{
		Logger::Error("error");
	}

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
				Utils::Find(uuid, filepathByUuid));
			r3d.mesh = mesh;
		}

		if (const auto& materialData = renderer3DData["Material"])
		{
			const std::string uuid = materialData.as<std::string>();
			const std::shared_ptr<Material> material = MaterialManager::GetInstance().LoadMaterial(
				Utils::Find(uuid, filepathByUuid));
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

void Serializer::SerializeDirectionalLight(YAML::Emitter& out, const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<DirectionalLight>())
	{
		return;
	}

	const DirectionalLight& directionalLight = entity->GetComponent<DirectionalLight>();

	out << YAML::Key << "DirectionalLight";

	out << YAML::BeginMap;

	out << YAML::Key << "Color" << YAML::Value << directionalLight.color;
	out << YAML::Key << "Intensity" << YAML::Value << directionalLight.intensity;

	out << YAML::EndMap;
}

void Serializer::DeserializeDirectionalLight(const YAML::Node& in, const std::shared_ptr<Entity>& entity)
{
	if (const auto& directionalLightData = in["DirectionalLight"])
	{
		if (!entity->HasComponent<DirectionalLight>())
		{
			entity->AddComponent<DirectionalLight>();
		}

		DirectionalLight& directionalLight = entity->GetComponent<DirectionalLight>();

		if (const auto& colorData = directionalLightData["Color"])
		{
			directionalLight.color = colorData.as<glm::vec3>();
		}

		if (const auto& intensityData = directionalLightData["Intensity"])
		{
			directionalLight.intensity = intensityData.as<float>();
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
	out << YAML::Key << "RenderPassName" << YAML::Value << camera.GetRenderPassName();
	out << YAML::Key << "RenderTargetIndex" << YAML::Value << camera.GetRenderTargetIndex();

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

		if (const auto& renderPassNameData = cameraData["RenderPassName"])
		{
			camera.SetRenderPassName(renderPassNameData.as<std::string>());
		}

		if (const auto& renderTargetIndexData = cameraData["RenderTargetIndex"])
		{
			camera.SetRenderTargetIndex(renderTargetIndexData.as<int>());
		}
	}
}

void Serializer::SerializeScene(const std::filesystem::path& filepath, const std::shared_ptr<Scene>& scene)
{
	if (!scene)
	{
		Logger::Error("Failed to serialize a scene, the scene is invalid!");
		return;
	}

	YAML::Emitter out;

	out << YAML::BeginMap;

	// Viewports.
	out << YAML::Key << "Viewports";
	out << YAML::Value << YAML::BeginSeq;

	for (const auto& [name, viewport] : ViewportManager::GetInstance().GetViewports())
	{
		if (const std::shared_ptr<Entity> camera = viewport->GetCamera().lock())
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Viewport" << YAML::Value << name;
			out << YAML::Key << "Camera" << YAML::Value << camera->GetUUID();
			out << YAML::EndMap;
		}
	}

	out << YAML::EndSeq;
	//

	// Settings.
	out << YAML::Key << "Settings";
	out << YAML::Value << YAML::BeginMap;

	out << YAML::Key << "DrawBoundingBoxes" << YAML::Value << scene->GetSettings().m_DrawBoundingBoxes;

	out << YAML::EndMap;
	//

	// Graphics Settings.
	const std::string& graphicsSettingsUuid = Utils::FindUuid(scene->GetGraphicsSettings().GetFilepath());
	if (!graphicsSettingsUuid.empty())
	{
		out << YAML::Key << "GraphicsSettings" << YAML::Value << graphicsSettingsUuid;
	}
	//

	// Entities.
	out << YAML::Key << "Scene";
	out << YAML::Value << YAML::BeginSeq;

	for (const auto& entity : scene->GetEntities())
	{
		SerializeEntity(out, entity, false);
	}

	out << YAML::EndSeq;
	//

	out << YAML::EndMap;

	std::ofstream fout(filepath);
	fout << out.c_str();
	fout.close();

	Logger::Log("Scene:" + filepath.string() + " has been serialized!", GREEN);
}

std::shared_ptr<Scene> Serializer::DeserializeScene(const std::filesystem::path& filepath)
{
	if (filepath.empty() || filepath == none || Utils::GetFileFormat(filepath) != FileFormats::Scene())
	{
		Logger::Error(filepath.string() + ":Failed to load scene! Filepath is incorrect!");
		return nullptr;
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath.string() + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
	}

	std::shared_ptr<Scene> scene = SceneManager::GetInstance().Create(
		Utils::GetFilename(filepath),
		"Main");
	scene->SetFilepath(filepath);

	std::unordered_map<std::shared_ptr<Entity>, std::vector<std::string>> childsUUIDByEntity;
	for (const auto& entityData : data["Scene"])
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

	for (const auto& viewportData : data["Viewports"])
	{
		std::shared_ptr<Entity> camera;
		if (const auto& cameraUuidData = viewportData["Camera"])
		{
			camera = scene->FindEntityByUUID(cameraUuidData.as<std::string>());
		}

		std::shared_ptr<Viewport> viewport;
		if (const auto& viewportNameData = viewportData["Viewport"])
		{
			viewport = ViewportManager::GetInstance().GetViewport(viewportNameData.as<std::string>());
		}

		if (viewport)
		{
			if (camera)
			{
				viewport->SetCamera(camera);
			}
		}
	}

	if (const auto& settingsData = data["Settings"])
	{
		if (const auto& drawBoundingBoxesData = settingsData["DrawBoundingBoxes"])
		{
			scene->GetSettings().m_DrawBoundingBoxes = drawBoundingBoxesData.as<bool>();
		}
	}

	if (const auto& graphicsSettingsUUIDData = data["GraphicsSettings"])
	{
		const std::filesystem::path& graphicsSettingsFilepath = Utils::Find(graphicsSettingsUUIDData.as<std::string>(), filepathByUuid);
		if (!graphicsSettingsFilepath.empty())
		{
			scene->SetGraphicsSettings(DeserializeGraphicsSettings(graphicsSettingsFilepath));
		}
	}

	Logger::Log("Scene:" + filepath.string() + " has been deserialized!", GREEN);

	return scene;
}

void Serializer::SerializeGraphicsSettings(const GraphicsSettings& graphicsSettings)
{
	YAML::Emitter out;

	out << YAML::BeginMap;

	// SSAO.
	out << YAML::Key << "SSAO";
	out << YAML::Value << YAML::BeginMap;

	out << YAML::Key << "IsEnabled" << YAML::Value << graphicsSettings.ssao.isEnabled;
	out << YAML::Key << "AoScale" << YAML::Value << graphicsSettings.ssao.aoScale;
	out << YAML::Key << "Bias" << YAML::Value << graphicsSettings.ssao.bias;
	out << YAML::Key << "KernelSize" << YAML::Value << graphicsSettings.ssao.kernelSize;
	out << YAML::Key << "NoiseSize" << YAML::Value << graphicsSettings.ssao.noiseSize;
	out << YAML::Key << "Radius" << YAML::Value << graphicsSettings.ssao.radius;

	out << YAML::EndMap;
	//

	// SSAO.
	out << YAML::Key << "CSM";
	out << YAML::Value << YAML::BeginMap;

	out << YAML::Key << "IsEnabled" << YAML::Value << graphicsSettings.shadows.isEnabled;
	out << YAML::Key << "CascadeCount" << YAML::Value << graphicsSettings.shadows.cascadeCount;
	out << YAML::Key << "SplitFactor" << YAML::Value << graphicsSettings.shadows.splitFactor;
	out << YAML::Key << "FogFactor" << YAML::Value << graphicsSettings.shadows.fogFactor;
	out << YAML::Key << "PcfEnabled" << YAML::Value << graphicsSettings.shadows.pcfEnabled;
	out << YAML::Key << "PcfRange" << YAML::Value << graphicsSettings.shadows.pcfRange;
	out << YAML::Key << "Biases" << YAML::Value << graphicsSettings.shadows.biases;

	out << YAML::EndMap;
	//

	out << YAML::EndMap;

	std::ofstream fout(graphicsSettings.GetFilepath());
	fout << out.c_str();
	fout.close();
}

GraphicsSettings Serializer::DeserializeGraphicsSettings(const std::filesystem::path& filepath)
{
	GraphicsSettings graphicsSettings(Utils::GetFilename(filepath), filepath);

	if (filepath.empty() || filepath == none || Utils::GetFileFormat(filepath) != FileFormats::GraphicsSettings())
	{
		Logger::Error(filepath.string() + ":Failed to load scene! Filepath is incorrect!");
		return graphicsSettings;
	}

	std::ifstream stream(filepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(filepath.string() + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
	}

	if (const auto& ssaoData = data["SSAO"])
	{
		if (const auto& isEnabledData = ssaoData["IsEnabled"])
		{
			graphicsSettings.ssao.isEnabled = isEnabledData.as<bool>();
		}

		if (const auto& aoScaleData = ssaoData["AoScale"])
		{
			graphicsSettings.ssao.aoScale = aoScaleData.as<float>();
		}

		if (const auto& biasData = ssaoData["Bias"])
		{
			graphicsSettings.ssao.bias = biasData.as<float>();
		}

		if (const auto& kernelSizeData = ssaoData["KernelSize"])
		{
			graphicsSettings.ssao.kernelSize = kernelSizeData.as<int>();
		}

		if (const auto& noiseSizeData = ssaoData["NoiseSize"])
		{
			graphicsSettings.ssao.noiseSize = noiseSizeData.as<int>();
		}

		if (const auto& radiusData = ssaoData["Radius"])
		{
			graphicsSettings.ssao.radius = radiusData.as<float>();
		}
	}

	if (const auto& csmData = data["CSM"])
	{
		if (const auto& isEnabledData = csmData["IsEnabled"])
		{
			graphicsSettings.shadows.isEnabled = isEnabledData.as<bool>();
		}

		if (const auto& cascadeCountData = csmData["CascadeCount"])
		{
			graphicsSettings.shadows.cascadeCount = cascadeCountData.as<int>();
		}

		if (const auto& splitFactorData = csmData["SplitFactor"])
		{
			graphicsSettings.shadows.splitFactor = splitFactorData.as<float>();
		}

		if (const auto& biasesData = csmData["Biases"])
		{
			graphicsSettings.shadows.biases = biasesData.as<std::vector<float>>();
		}

		if (const auto& fogFactorData = csmData["FogFactor"])
		{
			graphicsSettings.shadows.fogFactor = fogFactorData.as<float>();
		}

		if (const auto& pcfEnabledData = csmData["PcfEnabled"])
		{
			graphicsSettings.shadows.pcfEnabled = pcfEnabledData.as<bool>();
		}

		if (const auto& pcfRangeData = csmData["PcfRange"])
		{
			graphicsSettings.shadows.pcfRange = pcfRangeData.as<int>();
		}
	}

	return graphicsSettings;
}

void Serializer::ParseUniformValues(
	const YAML::detail::iterator_value& data,
	Pipeline::UniformInfo& uniformsInfo)
{
	for (const auto& uniformData : data["Uniforms"])
	{
		std::string uniformName;
		if (const auto& nameData = uniformData["Name"])
		{
			uniformName = nameData.as<std::string>();
		}

		if (uniformData["Values"])
		{
			Pipeline::UniformBufferInfo uniformBufferInfo{};

			for (const auto& valueData : uniformData["Values"])
			{
				std::string valueName;
				if (const auto& nameData = valueData["Name"])
				{
					valueName = nameData.as<std::string>();
				}

				ShaderReflection::ReflectVariable::Type valueType;
				if (const auto& typeData = valueData["Type"])
				{
					valueType = ShaderReflection::ConvertStringToType(typeData.as<std::string>());
				}
				else
				{
					Logger::Warning("Failed to load type of " + valueName);
					continue;
				}

				if (const auto& bufferValueData = valueData["Value"])
				{
					if (valueType == ShaderReflection::ReflectVariable::Type::INT)
					{
						uniformBufferInfo.intValuesByName.emplace(valueName, bufferValueData.as<int>());
					}
					else if (valueType == ShaderReflection::ReflectVariable::Type::FLOAT)
					{
						uniformBufferInfo.floatValuesByName.emplace(valueName, bufferValueData.as<float>());
					}
					else if (valueType == ShaderReflection::ReflectVariable::Type::VEC2)
					{
						uniformBufferInfo.vec2ValuesByName.emplace(valueName, bufferValueData.as<glm::vec2>());
					}
					else if (valueType == ShaderReflection::ReflectVariable::Type::VEC3)
					{
						uniformBufferInfo.vec3ValuesByName.emplace(valueName, bufferValueData.as<glm::vec3>());
					}
					else if (valueType == ShaderReflection::ReflectVariable::Type::VEC4)
					{
						uniformBufferInfo.vec4ValuesByName.emplace(valueName, bufferValueData.as<glm::vec4>());
					}
				}
			}
			uniformsInfo.uniformBuffersByName.emplace(uniformName, uniformBufferInfo);
		}

		if (const auto& nameData = uniformData["Value"])
		{
			const std::string textureFilepath = nameData.as<std::string>();

			if (textureFilepath.empty())
			{
				uniformsInfo.texturesByName.emplace(uniformName, TextureManager::GetInstance().GetWhite()->GetName());
			}
			else if (std::filesystem::exists(textureFilepath))
			{
				uniformsInfo.texturesByName.emplace(uniformName, textureFilepath);
			}
			else
			{
				uniformsInfo.texturesByName.emplace(uniformName, Utils::Find(textureFilepath, filepathByUuid).string());
			}
		}

		if (const auto& nameData = uniformData["RenderTarget"])
		{
			std::string renderTargetName = nameData.as<std::string>();
			size_t openBracketIndex = renderTargetName.find_first_of('[');
			if (openBracketIndex != std::string::npos)
			{
				size_t closeBracketIndex = renderTargetName.find_last_of(']');
				if (closeBracketIndex != std::string::npos)
				{
					UniformLayout::RenderTargetInfo renderTargetInfo{};

					const std::string attachmentIndexString = renderTargetName.substr(openBracketIndex + 1, closeBracketIndex - openBracketIndex - 1);
					renderTargetInfo.renderPassName = renderTargetName.substr(0, openBracketIndex);
					renderTargetInfo.attachmentIndex = std::stoul(attachmentIndexString);

					uniformsInfo.renderTargetsByName.emplace(uniformName, renderTargetInfo);
				}
				else
				{
					Logger::Warning(uniformName + ": no close bracket for render target!");
				}
			}
			else
			{
				Logger::Warning(uniformName + ": no attachment index for render target!");
			}
		}
	}
}

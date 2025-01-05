#include "Serializer.h"

#include "AsyncAssetLoader.h"
#include "FileFormatNames.h"
#include "Logger.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "RenderPassManager.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include "ThreadPool.h"
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

					Utils::SetUUID(uuid, shortFilepath);
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
	metaFilepath.concat(FileFormats::Meta()); 
	std::ofstream fout(metaFilepath);
	fout << out.c_str();
	fout.close();

	Utils::SetUUID(uuid, filepath);

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

		auto getShaderFilepath = [](const YAML::Node& node)
		{
			const std::string filepathOrUUID = node.as<std::string>();
			return std::filesystem::exists(filepathOrUUID) ? filepathOrUUID : Utils::FindFilepath(filepathOrUUID).string();
		};

		if (const auto& vertexData = pipelineData["Vertex"])
		{
			pipelineCreateInfo.shaderFilepathsByType[Pipeline::ShaderType::VERTEX] = getShaderFilepath(vertexData);
		}

		if (const auto& fragmentData = pipelineData["Fragment"])
		{
			pipelineCreateInfo.shaderFilepathsByType[Pipeline::ShaderType::FRAGMENT] = getShaderFilepath(fragmentData);
		}

		if (const auto& geometryData = pipelineData["Geometry"])
		{
			pipelineCreateInfo.shaderFilepathsByType[Pipeline::ShaderType::GEOMETRY] = getShaderFilepath(geometryData);
		}

		if (const auto& computeData = pipelineData["Compute"])
		{
			pipelineCreateInfo.shaderFilepathsByType[Pipeline::ShaderType::COMPUTE] = getShaderFilepath(computeData);
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
			Pipeline::BlendStateAttachment blendStateAttachment{};
			
			if (const auto& blendEnabledData = colorBlendStateData["BlendEnabled"])
			{
				blendStateAttachment.blendEnabled = blendEnabledData.as<bool>();
			}
			
			auto blendOpParse = [](const std::string& blendOpType)
			{
				if (blendOpType == "add")
				{
					return Pipeline::BlendStateAttachment::BlendOp::ADD;
				}
				else if (blendOpType == "subtract")
				{
					return Pipeline::BlendStateAttachment::BlendOp::SUBTRACT;
				}
				else if (blendOpType == "reverse subtract")
				{
					return Pipeline::BlendStateAttachment::BlendOp::REVERSE_SUBTRACT;
				}
				else if (blendOpType == "max")
				{
					return Pipeline::BlendStateAttachment::BlendOp::MAX;
				}
				else if (blendOpType == "min")
				{
					return Pipeline::BlendStateAttachment::BlendOp::MIN;
				}
			};

			auto blendFactorParse = [](const std::string& blendFactorType)
			{
				if (blendFactorType == "constant alpha")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::CONSTANT_ALPHA;
				}
				else if (blendFactorType == "constant color")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::CONSTANT_COLOR;
				}
				else if (blendFactorType == "dst alpha")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::DST_ALPHA;
				}
				else if (blendFactorType == "dst color")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::DST_COLOR;
				}
				else if (blendFactorType == "one")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::ONE;
				}
				else if (blendFactorType == "one minus constant alpha")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::ONE_MINUS_CONSTANT_ALPHA;
				}
				else if (blendFactorType == "one minus constant color")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::ONE_MINUS_CONSTANT_COLOR;
				}
				else if (blendFactorType == "one minus dst alpha")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::ONE_MINUS_DST_ALPHA;
				}
				else if (blendFactorType == "one minus dst color")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::ONE_MINUS_DST_COLOR;
				}
				else if (blendFactorType == "one minus src alpha")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::ONE_MINUS_SRC_ALPHA;
				}
				else if (blendFactorType == "one minus src color")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::ONE_MINUS_SRC_COLOR;
				}
				else if (blendFactorType == "src alpha")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::SRC_ALPHA;
				}
				else if (blendFactorType == "src alpha saturate")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::SRC_ALPHA_SATURATE;
				}
				else if (blendFactorType == "src color")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::SRC_COLOR;
				}
				else if (blendFactorType == "zero")
				{
					return Pipeline::BlendStateAttachment::BlendFactor::ZERO;
				}
			};

			if (const auto& blendOpData = colorBlendStateData["ColorBlendOp"])
			{
				blendStateAttachment.colorBlendOp = blendOpParse(blendOpData.as<std::string>());
			}

			if (const auto& blendOpData = colorBlendStateData["AlphaBlendOp"])
			{
				blendStateAttachment.alphaBlendOp = blendOpParse(blendOpData.as<std::string>());
			}

			if (const auto& srcBlendFactorData = colorBlendStateData["SrcColorBlendFactor"])
			{
				blendStateAttachment.srcColorBlendFactor = blendFactorParse(srcBlendFactorData.as<std::string>());
			}

			if (const auto& dstBlendFactorData = colorBlendStateData["DstColorBlendFactor"])
			{
				blendStateAttachment.dstColorBlendFactor = blendFactorParse(dstBlendFactorData.as<std::string>());
			}

			if (const auto& srcBlendFactorData = colorBlendStateData["SrcAlphaBlendFactor"])
			{
				blendStateAttachment.srcAlphaBlendFactor = blendFactorParse(srcBlendFactorData.as<std::string>());
			}

			if (const auto& dstBlendFactorData = colorBlendStateData["DstAlphaBlendFactor"])
			{
				blendStateAttachment.dstAlphaBlendFactor = blendFactorParse(dstBlendFactorData.as<std::string>());
			}

			pipelineCreateInfo.colorBlendStateAttachments.emplace_back(blendStateAttachment);
		}

		ParseUniformValues(pipelineData, pipelineCreateInfo.uniformInfo);

		pipelineCreateInfos.emplace_back(pipelineCreateInfo);
	}

	Logger::Log("BaseMaterial:" + filepath.string() + " has been deserialized!", BOLDGREEN);

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
		const std::string filepathOrUUID = baseMaterialData.as<std::string>();
		if (std::filesystem::exists(filepathOrUUID))
		{
			createInfo.baseMaterial = AsyncAssetLoader::GetInstance().SyncLoadBaseMaterial(filepathOrUUID);
		}
		else
		{
			const std::filesystem::path baseMaterialFilepath = Utils::FindFilepath(filepathOrUUID);
			createInfo.baseMaterial = AsyncAssetLoader::GetInstance().SyncLoadBaseMaterial(baseMaterialFilepath);
		}

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

	Logger::Log("Material:" + filepath.string() + " has been deserialized!", BOLDGREEN);

	return createInfo;
}

void Serializer::SerializeMaterial(const std::shared_ptr<Material>& material, bool useLog)
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
			out << YAML::EndSeq;

			out << YAML::EndMap;

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

	if (useLog)
	{
		Logger::Log("Material:" + material->GetFilepath().string() + " has been serialized!", BOLDGREEN);
	}
}

void Serializer::SerializeMesh(const std::filesystem::path& directory,  const std::shared_ptr<Mesh>& mesh)
{
	const std::string meshName = mesh->GetName();

	size_t dataSize = mesh->GetVertexCount() * mesh->GetVertexSize() +
		mesh->GetVertexLayouts().size() * sizeof(VertexLayout) +
		mesh->GetIndexCount() * sizeof(uint32_t) +
		mesh->GetName().size() +
		mesh->GetFilepath().string().size() +
		6 * 4; // Vertex Count, Vertex Size, Index Count, Mesh Size, Filepath Size, Vertex Layout Count.

	uint32_t offset = 0;

	uint8_t* data = new uint8_t[dataSize];

	// Name.
	{
		Utils::GetValue<uint32_t>(data, offset) = static_cast<uint32_t>(meshName.size());
		offset += sizeof(uint32_t);

		memcpy(&Utils::GetValue<uint8_t>(data, offset), meshName.data(), meshName.size());
		offset += static_cast<uint32_t>(meshName.size());
	}

	// Vertices.
	{
		Utils::GetValue<uint32_t>(data, offset) = mesh->GetVertexCount();
		offset += sizeof(uint32_t);
		Utils::GetValue<uint32_t>(data, offset) = mesh->GetVertexSize();
		offset += sizeof(uint32_t);

		const void* vertices = mesh->GetRawVertices();
		memcpy(&Utils::GetValue<uint8_t>(data, offset), vertices, mesh->GetVertexCount() * mesh->GetVertexSize());
		offset += mesh->GetVertexCount() * mesh->GetVertexSize();
	}

	// Vertex Layouts.
	{
		Utils::GetValue<uint32_t>(data, offset) = mesh->GetVertexLayouts().size();
		offset += sizeof(uint32_t);

		for (const VertexLayout& vertexLayout : mesh->GetVertexLayouts())
		{
			memcpy(&Utils::GetValue<uint8_t>(data, offset), &vertexLayout.size, sizeof(uint32_t));
			offset += sizeof(uint32_t);
		}
	}

	// Indices.
	{
		Utils::GetValue<uint32_t>(data, offset) = mesh->GetIndexCount();
		offset += sizeof(uint32_t);

		// Potential optimization of using 2 bit instead of 4 bit.
		std::vector<uint32_t> indices = mesh->GetRawIndices();
		memcpy(&Utils::GetValue<uint8_t>(data, offset), indices.data(), mesh->GetIndexCount() * sizeof(uint32_t));
		offset += mesh->GetIndexCount() * sizeof(uint32_t);
	}

	std::filesystem::path outMeshFilepath = directory / (meshName + FileFormats::Mesh());
	std::ofstream out(outMeshFilepath, std::ostream::binary);

	out.write((char*)data, static_cast<std::streamsize>(dataSize));

	delete[] data;

	out.close();

	GenerateFileUUID(mesh->GetFilepath());

	Logger::Log("Mesh:" + outMeshFilepath.string() + " has been serialized!", BOLDGREEN);
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

	uint8_t* data = new uint8_t[size];

	in.read((char*)data, size);

	in.close();

	int offset = 0;

	std::string meshName;
	// Name.
	{
		const uint32_t meshNameSize = Utils::GetValue<uint32_t>(data, offset);
		offset += sizeof(uint32_t);

		meshName.resize(meshNameSize);
		memcpy(meshName.data(), &Utils::GetValue<uint8_t>(data, offset), meshNameSize);
		offset += meshNameSize;
	}

	// Vertices.
	uint8_t* vertices;
	const uint32_t vertexCount = Utils::GetValue<uint32_t>(data, offset);
	offset += sizeof(uint32_t);
	const uint32_t vertexSize = Utils::GetValue<uint32_t>(data, offset);
	offset += sizeof(uint32_t);
	{
		vertices = new uint8_t[vertexCount * vertexSize];
		memcpy(vertices, &Utils::GetValue<uint8_t>(data, offset), vertexCount * vertexSize);
		offset += vertexCount * vertexSize;
	}

	// Vertex Layouts.
	{
		const uint32_t vertexLayoutCount = Utils::GetValue<uint32_t>(data, offset);
		offset += sizeof(uint32_t);

		std::vector<VertexLayout> vertexLayouts;
		for (size_t i = 0; i < vertexLayoutCount; i++)
		{
			VertexLayout vertexLayout = vertexLayouts.emplace_back();
			memcpy(&vertexLayout, &Utils::GetValue<uint32_t>(data, offset), sizeof(uint32_t));
			offset += sizeof(uint32_t);
		}
	}

	std::vector<uint32_t> indices;
	// Indices.
	{
		const uint32_t indicesSize = Utils::GetValue<uint32_t>(data, offset);
		offset += sizeof(uint32_t);

		indices.resize(indicesSize);
		memcpy(indices.data(), &Utils::GetValue<uint8_t>(data, offset), indicesSize * sizeof(uint32_t));
		offset += indicesSize * sizeof(uint32_t);
	}

	delete[] data;

	Logger::Log("Mesh:" + filepath.string() + " has been deserialized!", BOLDGREEN);

	Mesh::CreateInfo createInfo{};
	createInfo.filepath = filepath;
	createInfo.name = meshName;
	createInfo.indices = indices;
	createInfo.vertices = vertices;
	createInfo.vertexCount = vertexCount;
	createInfo.vertexSize = vertexSize;
	createInfo.vertexLayouts =
	{
		VertexLayout(sizeof(VertexPosition)),
		VertexLayout(sizeof(VertexNormal))
	};

	return std::make_shared<Mesh>(createInfo);
}

void Serializer::SerializeShaderCache(const std::filesystem::path& filepath, const std::string& code)
{
	const std::filesystem::path directory = "Shaders\\Cache\\";

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

	const std::filesystem::path directory = "Shaders\\Cache\\";
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

void Serializer::SerializeShaderModuleReflection(
	const std::filesystem::path& filepath,
	const ShaderReflection::ReflectShaderModule& reflectShaderModule)
{
	std::function<void(YAML::Emitter&, const std::vector<ShaderReflection::ReflectVariable>&)> serializeVariables;

	serializeVariables = [&serializeVariables](YAML::Emitter& out, const std::vector<ShaderReflection::ReflectVariable>& variables)
	{
		out << YAML::Key << "Variables";

		out << YAML::BeginSeq;

		for (const auto& variable : variables)
		{
			out << YAML::BeginMap;
		
			out << YAML::Key << "Name" << YAML::Value << variable.name;
			out << YAML::Key << "Count" << YAML::Value << variable.count;
			out << YAML::Key << "Offset" << YAML::Value << variable.offset;
			out << YAML::Key << "Size" << YAML::Value << variable.size;
			out << YAML::Key << "Type" << YAML::Value << ShaderReflection::ConvertTypeToString(variable.type);

			serializeVariables(out, variable.variables);

			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
	};

	const std::filesystem::path directory = "Shaders\\Cache\\";

	if (!std::filesystem::exists(directory))
	{
		std::filesystem::create_directories(directory);
	}

	std::filesystem::path reflectShaderModuleFilepath = directory / filepath.filename();
	reflectShaderModuleFilepath.concat(FileFormats::Refl());

	const size_t lastWriteTime = std::filesystem::last_write_time(filepath).time_since_epoch().count();

	YAML::Emitter out;

	out << YAML::BeginMap;
	
	out << YAML::Key << "LastWriteTime" << YAML::Value << lastWriteTime;

	out << YAML::Key << "ReflectShaderModule";

	out << YAML::BeginMap;

	out << YAML::Key << "SetLayouts";

	out << YAML::BeginSeq;

	for (const auto& setLayout : reflectShaderModule.setLayouts)
	{
		out << YAML::BeginMap;

		out << YAML::Key << "Set" << YAML::Value << setLayout.set;

		out << YAML::Key << "Bindings";

		out << YAML::BeginSeq;

		for (const auto& binding : setLayout.bindings)
		{
			out << YAML::BeginMap;

			out << YAML::Key << "Name" << YAML::Value << binding.name;
			out << YAML::Key << "Binding" << YAML::Value << binding.binding;
			out << YAML::Key << "Count" << YAML::Value << binding.count;
			out << YAML::Key << "Type" << YAML::Value << (int)binding.type;

			if (binding.buffer)
			{
				out << YAML::Key << "Buffer";

				out << YAML::BeginMap;

				out << YAML::Key << "Name" << YAML::Value << binding.buffer->name;
				out << YAML::Key << "Count" << YAML::Value << binding.buffer->count;
				out << YAML::Key << "Offset" << YAML::Value << binding.buffer->offset;
				out << YAML::Key << "Size" << YAML::Value << binding.buffer->size;
				out << YAML::Key << "Type" << YAML::Value << ShaderReflection::ConvertTypeToString(binding.buffer->type);

				serializeVariables(out, binding.buffer->variables);

				out << YAML::EndMap;
			}

			out << YAML::EndMap;
		}

		out << YAML::EndSeq;

		out << YAML::EndMap;
	}

	out << YAML::EndSeq;

	out << YAML::Key << "AttributeDescriptions";

	out << YAML::BeginSeq;

	for (const auto& attributeDescription : reflectShaderModule.attributeDescriptions)
	{
		out << YAML::BeginMap;

		out << YAML::Key << "Count" << YAML::Value << attributeDescription.count;
		out << YAML::Key << "Format" << YAML::Value << (int)attributeDescription.format;
		out << YAML::Key << "Location" << YAML::Value << attributeDescription.location;
		out << YAML::Key << "Name" << YAML::Value << attributeDescription.name;
		out << YAML::Key << "Size" << YAML::Value << attributeDescription.size;

		out << YAML::EndMap;
	}

	out << YAML::EndSeq;

	out << YAML::EndMap;

	out << YAML::EndMap;

	std::ofstream fout(reflectShaderModuleFilepath);
	fout << out.c_str();
	fout.close();
}

std::optional<ShaderReflection::ReflectShaderModule> Serializer::DeserializeShaderModuleReflection(const std::filesystem::path& filepath)
{
	if (filepath.empty())
	{
		return std::nullopt;
	}

	const std::filesystem::path directory = "Shaders\\Cache\\";
	std::filesystem::path reflectShaderModuleFilepath = directory / filepath.filename();
	reflectShaderModuleFilepath.concat(FileFormats::Refl());

	std::ifstream stream(reflectShaderModuleFilepath);
	std::stringstream stringStream;

	stringStream << stream.rdbuf();

	stream.close();

	YAML::Node data = YAML::LoadMesh(stringStream.str());
	if (!data)
	{
		FATAL_ERROR(reflectShaderModuleFilepath.string() + ":Failed to load yaml file! The file doesn't contain data or doesn't exist!");
	}

	size_t lastWriteTime = 0;
	if (const auto& lastWriteTimeData = data["LastWriteTime"])
	{
		lastWriteTime = lastWriteTimeData.as<size_t>();
	}

	if (std::filesystem::exists(reflectShaderModuleFilepath))
	{
		if (lastWriteTime != std::filesystem::last_write_time(filepath).time_since_epoch().count())
		{
			return std::nullopt;
		}
	}
	else
	{
		return std::nullopt;
	}

	std::function<void(const YAML::Node&, std::vector<ShaderReflection::ReflectVariable>&)> deserializeVariables;

	deserializeVariables = [&deserializeVariables](const YAML::Node& node, std::vector<ShaderReflection::ReflectVariable>& variables)
	{
		for (const auto& variableData : node)
		{
			ShaderReflection::ReflectVariable& reflectVariable = variables.emplace_back();

			if (const auto& nameData = variableData["Name"])
			{
				reflectVariable.name = nameData.as<std::string>();
			}

			if (const auto& countData = variableData["Count"])
			{
				reflectVariable.count = countData.as<uint32_t>();
			}

			if (const auto& offsetData = variableData["Offset"])
			{
				reflectVariable.offset = offsetData.as<uint32_t>();
			}

			if (const auto& sizeData = variableData["Size"])
			{
				reflectVariable.size = sizeData.as<uint32_t>();
			}

			if (const auto& typeData = variableData["Type"])
			{
				reflectVariable.type = ShaderReflection::ConvertStringToType(typeData.as<std::string>());
			}

			if (const auto& variablesData = variableData["Variables"])
			{
				deserializeVariables(variablesData, reflectVariable.variables);
			}
		}
	};

	ShaderReflection::ReflectShaderModule reflectShaderModule{};
	if (const auto& reflectShaderModuleData = data["ReflectShaderModule"])
	{
		for (const auto& setLayoutData : reflectShaderModuleData["SetLayouts"])
		{
			ShaderReflection::ReflectDescriptorSetLayout& setLayout = reflectShaderModule.setLayouts.emplace_back();
			
			if (const auto& setData = setLayoutData["Set"])
			{
				setLayout.set = setData.as<uint32_t>();
			}

			for (const auto& bindingData : setLayoutData["Bindings"])
			{
				ShaderReflection::ReflectDescriptorSetBinding& binding = setLayout.bindings.emplace_back();

				if (const auto& nameData = bindingData["Name"])
				{
					binding.name = nameData.as<std::string>();
				}

				if (const auto& locationData = bindingData["Binding"])
				{
					binding.binding = locationData.as<uint32_t>();
				}

				if (const auto& countData = bindingData["Count"])
				{
					binding.count = countData.as<uint32_t>();
				}

				if (const auto& typeData = bindingData["Type"])
				{
					binding.type = (ShaderReflection::Type)typeData.as<int>();
				}

				if (const auto& bufferData = bindingData["Buffer"])
				{
					ShaderReflection::ReflectVariable reflectVariable{};

					if (const auto& nameData = bufferData["Name"])
					{
						reflectVariable.name = nameData.as<std::string>();
					}

					if (const auto& countData = bufferData["Count"])
					{
						reflectVariable.count = countData.as<uint32_t>();
					}

					if (const auto& offsetData = bufferData["Offset"])
					{
						reflectVariable.offset = offsetData.as<uint32_t>();
					}

					if (const auto& sizeData = bufferData["Size"])
					{
						reflectVariable.size = sizeData.as<uint32_t>();
					}

					if (const auto& typeData = bufferData["Type"])
					{
						reflectVariable.type = ShaderReflection::ConvertStringToType(typeData.as<std::string>());
					}

					if (const auto& variablesData = bufferData["Variables"])
					{
						deserializeVariables(variablesData, reflectVariable.variables);
					}

					binding.buffer = reflectVariable;
				}
			}
		}

		for (const auto& attributeDescriptionData : reflectShaderModuleData["AttributeDescriptions"])
		{
			ShaderReflection::AttributeDescription& attributeDescription = reflectShaderModule.attributeDescriptions.emplace_back();

			if (const auto& countData = attributeDescriptionData["Count"])
			{
				attributeDescription.count = countData.as<uint32_t>();
			}

			if (const auto& formatData = attributeDescriptionData["Format"])
			{
				attributeDescription.format = (Format)formatData.as<int>();
			}

			if (const auto& locationData = attributeDescriptionData["Location"])
			{
				attributeDescription.location = locationData.as<uint32_t>();
			}

			if (const auto& nameData = attributeDescriptionData["Name"])
			{
				attributeDescription.name = nameData.as<std::string>();
			}

			if (const auto& sizeData = attributeDescriptionData["Size"])
			{
				attributeDescription.size = sizeData.as<uint32_t>();
			}
		}
	}

	return reflectShaderModule;
}

std::unordered_map<std::shared_ptr<Material>, std::vector<std::shared_ptr<Mesh>>>
Serializer::LoadIntermediate(
	const std::filesystem::path& filepath,
	std::string& workName,
	float& workStatus)
{
	workName = "Loading " + filepath.string();

	const std::filesystem::path directory = filepath.parent_path();

	Assimp::Importer import;
	constexpr auto importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices |
		aiProcess_OptimizeMeshes | aiProcess_RemoveRedundantMaterials | aiProcess_ImproveCacheLocality |
		aiProcess_JoinIdenticalVertices | aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | aiProcess_SortByPType | aiProcess_FindInstances |
		aiProcess_ValidateDataStructure | aiProcess_FindDegenerates | aiProcess_FindInvalidData;
	const aiScene* scene = import.ReadFile(filepath.string(), importFlags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		FATAL_ERROR(std::string("ASSIMP::" + std::string(import.GetErrorString())).c_str());
	}

	const float maxWorkStatus = scene->mNumMaterials + scene->mNumMeshes;
	float currentWorkStatus = 0.0f;

	workName = "Generating Materials";
	std::mutex futureMaterialMutex;
	std::condition_variable futureMaterialCondVar;
	std::atomic<int> materialCount = 0;
	std::unordered_map<size_t, std::shared_ptr<Material>> materialsByIndex;
	for (size_t materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++)
	{
		ThreadPool::GetInstance().EnqueueAsync([
			aiMaterial = scene->mMaterials[materialIndex],
				directory,
				materialIndex,
				maxWorkStatus,
				&materialCount,
				&futureMaterialMutex,
				&futureMaterialCondVar,
				&materialsByIndex,
				&workStatus,
				&currentWorkStatus]()
		{
			std::shared_ptr<Material> material = GenerateMaterial(aiMaterial, directory);

			std::lock_guard<std::mutex> lock(futureMaterialMutex);

			workStatus = currentWorkStatus++ / maxWorkStatus;
			
			materialsByIndex[materialIndex] = material;

			materialCount.store(materialsByIndex.size());

			futureMaterialCondVar.notify_all();
		});
	}

	std::mutex futureMaterialCondVarMutex;
	std::unique_lock<std::mutex> lock(futureMaterialCondVarMutex);
	futureMaterialCondVar.wait(lock, [&materialCount, scene]
	{
		return materialCount.load() == scene->mNumMaterials;
	});

	workName = "Generating Meshes";
	std::vector<std::shared_ptr<Mesh>> meshesByIndex(scene->mNumMeshes);
	std::unordered_map<std::shared_ptr<Mesh>, std::shared_ptr<Material>> materialsByMeshes;
	for (size_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
	{
		aiMesh* aiMesh = scene->mMeshes[meshIndex];
		std::shared_ptr<Mesh> mesh = GenerateMesh(aiMesh, directory);
		meshesByIndex[meshIndex] = mesh;
		materialsByMeshes[mesh] = materialsByIndex[aiMesh->mMaterialIndex];

		workStatus = currentWorkStatus++ / maxWorkStatus;
	}

	if (scene->mNumMeshes > 1)
	{
		workName = "Generating Prefab";

		std::shared_ptr<Scene> GenerateGameObjectScene = SceneManager::GetInstance().Create("GenerateGameObject", "GenerateGameObject");
		std::shared_ptr<Entity> root = GenerateEntity(scene->mRootNode, GenerateGameObjectScene, meshesByIndex, materialsByMeshes);
		
		std::filesystem::path prefabFilepath = directory / root->GetName();
		prefabFilepath.replace_extension(FileFormats::Prefab());
		SerializePrefab(prefabFilepath, root);

		SceneManager::GetInstance().Delete(GenerateGameObjectScene);
	}
	
	return {};
}

std::shared_ptr<Mesh> Serializer::GenerateMesh(aiMesh* aiMesh, const std::filesystem::path& directory)
{
	VertexDefault* vertices = new VertexDefault[aiMesh->mNumVertices];
	std::vector<uint32_t> indices;

	std::string defaultMeshName = aiMesh->mName.C_Str();
	std::string meshName = defaultMeshName;
	meshName.erase(std::remove(meshName.begin(), meshName.end(), ':'), meshName.end());

	std::filesystem::path meshFilepath = directory / (meshName + FileFormats::Mesh());

	int containIndex = 0;
	while (MeshManager::GetInstance().GetMesh(meshFilepath))
	{
		meshName = defaultMeshName + "_" + std::to_string(containIndex);
		meshFilepath.replace_filename(meshName);
		containIndex++;
	}

	VertexDefault* vertex = vertices;
	for (size_t vertexIndex = 0; vertexIndex < aiMesh->mNumVertices; vertexIndex++, vertex++)
	{
		aiVector3D position = aiMesh->mVertices[vertexIndex];
		vertex->position.x = position.x;
		vertex->position.y = position.y;
		vertex->position.z = position.z;

		if (aiMesh->HasTextureCoords(0))
		{
			aiVector3D uv = aiMesh->mTextureCoords[0][vertexIndex];
			vertex->uv.x = uv.x;
			vertex->uv.y = uv.y;
		}
		else
		{
			vertex->uv.x = 0.0f;
			vertex->uv.y = 0.0f;
		}

		aiVector3D normal = aiMesh->mNormals[vertexIndex];
		vertex->normal.x = normal.x;
		vertex->normal.y = normal.y;
		vertex->normal.z = normal.z;

		if (aiMesh->HasTangentsAndBitangents())
		{
			// Tangent.
			aiVector3D tangent = aiMesh->mTangents[vertexIndex];
			vertex->tangent.x = tangent.x;
			vertex->tangent.y = tangent.y;
			vertex->tangent.z = tangent.z;

			// Bitngent.
			aiVector3D bitangent = aiMesh->mBitangents[vertexIndex];
			vertex->bitangent.x = bitangent.x;
			vertex->bitangent.y = bitangent.y;
			vertex->bitangent.z = bitangent.z;
		}
		else
		{
			// Tangent.
			vertex->tangent.x = 0.0f;
			vertex->tangent.y = 0.0f;
			vertex->tangent.z = 0.0f;

			// Bitngent.
			vertex->bitangent.x = 0.0f;
			vertex->bitangent.y = 0.0f;
			vertex->bitangent.z = 0.0f;
		}
	}

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

			vertices[i0].tangent.x = tangent.x;
			vertices[i0].tangent.y = tangent.y;
			vertices[i0].tangent.z = tangent.z;
			vertices[i0].bitangent.x = bitangent.x;
			vertices[i0].bitangent.y = bitangent.y;
			vertices[i0].bitangent.z = bitangent.z;

			vertices[i1].tangent.x = tangent.x;
			vertices[i1].tangent.y = tangent.y;
			vertices[i1].tangent.z = tangent.z;
			vertices[i1].bitangent.x = bitangent.x;
			vertices[i1].bitangent.y = bitangent.y;
			vertices[i1].bitangent.z = bitangent.z;

			vertices[i2].tangent.x = tangent.x;
			vertices[i2].tangent.y = tangent.y;
			vertices[i2].tangent.z = tangent.z;
			vertices[i2].bitangent.x = bitangent.x;
			vertices[i2].bitangent.y = bitangent.y;
			vertices[i2].bitangent.z = bitangent.z;
		}
	}

	Mesh::CreateInfo createInfo{};
	createInfo.filepath = std::move(meshFilepath);
	createInfo.name = std::move(meshName);
	createInfo.indices = std::move(indices);
	createInfo.vertices = vertices;
	createInfo.vertexCount = aiMesh->mNumVertices;
	createInfo.vertexSize = sizeof(VertexDefault);
	createInfo.vertexLayouts =
	{
		VertexLayout(sizeof(VertexPosition)),
		VertexLayout(sizeof(VertexNormal))
	};

	std::shared_ptr<Mesh> mesh = MeshManager::GetInstance().CreateMesh(createInfo);
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
		return AsyncAssetLoader::GetInstance().SyncLoadMaterial("Materials/MeshBase.mat");
	}

	std::filesystem::path materialFilepath = directory / materialName;
	materialFilepath.concat(FileFormats::Mat());

	const std::shared_ptr<Material> meshBaseMaterial = AsyncAssetLoader::GetInstance().SyncLoadMaterial("Materials/MeshBase.mat");
	const std::shared_ptr<Material> meshBaseDoubleSidedMaterial = AsyncAssetLoader::GetInstance().SyncLoadMaterial("Materials/MeshBaseDoubleSided.mat");

	// Maybe use later.
	bool doubleSided = false;
	aiMaterial->Get(AI_MATKEY_TWOSIDED, doubleSided);

	const std::shared_ptr<Material> material = MaterialManager::GetInstance().Clone(
		materialName,
		materialFilepath,
		doubleSided ? meshBaseDoubleSidedMaterial : meshBaseMaterial);

	const std::shared_ptr<UniformWriter> uniformWriter = material->GetUniformWriter(GBuffer);
	
	aiColor3D aiBaseColor(1.0f, 1.0f, 1.0f);
	if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiBaseColor) == aiReturn_SUCCESS)
	{
		glm::vec4 color = { aiBaseColor.r, aiBaseColor.g, aiBaseColor.b, 1.0f };
		material->WriteToBuffer("GBufferMaterial", "material.albedoColor", color);
	}
	
	aiColor3D aiEmissiveColor(1.0f, 1.0f, 1.0f);
	if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmissiveColor) == aiReturn_SUCCESS)
	{
		glm::vec4 color = { aiEmissiveColor.r, aiEmissiveColor.g, aiEmissiveColor.b, 1.0f };
		material->WriteToBuffer("GBufferMaterial", "material.emissiveColor", color);
	}

	float metallic = 0.0f;
	if (aiMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS)
	{
		material->WriteToBuffer("GBufferMaterial", "material.metallicFactor", metallic);
	}

	float specular = 0.0f;
	if (aiMaterial->Get(AI_MATKEY_SPECULAR_FACTOR, specular) == aiReturn_SUCCESS)
	{
		material->WriteToBuffer("GBufferMaterial", "material.metallicFactor", specular);
	}
	
	float roughness = 1.0f;
	if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
	{
		material->WriteToBuffer("GBufferMaterial", "material.roughnessFactor", roughness);
	}

	float glossines = 0.0f;
	if (aiMaterial->Get(AI_MATKEY_GLOSSINESS_FACTOR, glossines) == aiReturn_SUCCESS)
	{
		roughness = 1.0f - glossines;
		material->WriteToBuffer("GBufferMaterial", "material.roughnessFactor", roughness);
	}

	float ao = 1.0f;
	material->WriteToBuffer("GBufferMaterial", "material.aoFactor", ao);

	// If has no emissive intensity, just use r channel from emissive color.
	float emissiveFactor = 1.0f;
	if (aiMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveFactor) == aiReturn_SUCCESS)
	{
		material->WriteToBuffer("GBufferMaterial", "material.emissiveFactor", emissiveFactor);
	}
	else
	{
		material->WriteToBuffer("GBufferMaterial", "material.emissiveFactor", aiEmissiveColor.r);
	}

	uint32_t numTextures = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
	aiString aiTextureName;
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("albedoTexture", AsyncAssetLoader::GetInstance().SyncLoadTexture(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_BASE_COLOR);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_BASE_COLOR, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("albedoTexture", AsyncAssetLoader::GetInstance().SyncLoadTexture(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_NORMALS);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		Texture::Meta meta = DeserializeTextureMeta(textureFilepath.string() + FileFormats::Meta());
		meta.srgb = false;
		SerializeTextureMeta(meta);

		uniformWriter->WriteTexture("normalTexture", AsyncAssetLoader::GetInstance().SyncLoadTexture(textureFilepath));

		constexpr int useNormalMap = 1;
		material->WriteToBuffer("GBufferMaterial", "material.useNormalMap", useNormalMap);
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_METALNESS);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("metalnessTexture", AsyncAssetLoader::GetInstance().SyncLoadTexture(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_SPECULAR);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("metalnessTexture", AsyncAssetLoader::GetInstance().SyncLoadTexture(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("roughnessTexture", AsyncAssetLoader::GetInstance().SyncLoadTexture(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_SHININESS);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_SHININESS, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("roughnessTexture", AsyncAssetLoader::GetInstance().SyncLoadTexture(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_AMBIENT_OCCLUSION, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("aoTexture", AsyncAssetLoader::GetInstance().SyncLoadTexture(textureFilepath));
	}

	numTextures = aiMaterial->GetTextureCount(aiTextureType_EMISSIVE);
	if (numTextures > 0)
	{
		aiMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0), aiTextureName);
		std::filesystem::path textureFilepath = directory / aiTextureName.C_Str();

		uniformWriter->WriteTexture("emissiveTexture", AsyncAssetLoader::GetInstance().SyncLoadTexture(textureFilepath));
	}

	material->GetBuffer("GBufferMaterial")->Flush();
	uniformWriter->Flush();

	Material::Save(material);

	return material;
}

std::shared_ptr<Entity> Serializer::GenerateEntity(
	const aiNode* assimpNode,
	const std::shared_ptr<Scene>& scene,
	const std::vector<std::shared_ptr<Mesh>>& meshesByIndex,
	const std::unordered_map<std::shared_ptr<Mesh>, std::shared_ptr<Material>>& materialsByMeshes)
{
	if (!assimpNode)
	{
		return nullptr;
	}

	std::shared_ptr<Entity> node = scene->CreateEntity(assimpNode->mName.C_Str());
	Transform& nodeTransform = node->AddComponent<Transform>(node);

	Renderer3D* r3d = nullptr;
	if (assimpNode->mNumMeshes > 0)
	{
		r3d = &node->AddComponent<Renderer3D>();
	}

	for (size_t meshIndex = 0; meshIndex < assimpNode->mNumMeshes; meshIndex++)
	{
		const std::shared_ptr<Mesh> mesh = meshesByIndex[assimpNode->mMeshes[meshIndex]];
		r3d->mesh = mesh;
		r3d->material = materialsByMeshes.find(mesh)->second;
	}

	for (size_t childIndex = 0; childIndex < assimpNode->mNumChildren; childIndex++)
	{
		const aiNode* child = assimpNode->mChildren[childIndex];
		node->AddChild(GenerateEntity(child, scene, meshesByIndex, materialsByMeshes));
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
	for (const std::weak_ptr<Entity> weakChild : entity->GetChilds())
	{
		if (const std::shared_ptr<Entity> child = weakChild.lock())
		{
			childUUIDs.emplace_back(child->GetUUID());
		}
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

	for (const std::weak_ptr<Entity> weakChild : entity->GetChilds())
	{
		if (const std::shared_ptr<Entity> child = weakChild.lock())
		{
			SerializeEntity(out, child);
		}
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

	Logger::Log("Prefab:" + filepath.string() + " has been deserialized!", BOLDGREEN);
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
			AsyncAssetLoader::GetInstance().AsyncLoadMesh(Utils::FindFilepath(uuid), [wEntity = std::weak_ptr(entity)](std::shared_ptr<Mesh> mesh)
			{
				if (std::shared_ptr<Entity> entity = wEntity.lock())
				{
					if (entity->HasComponent<Renderer3D>())
					{
						entity->GetComponent<Renderer3D>().mesh = mesh;
					}
				}
			});
		}

		if (const auto& materialData = renderer3DData["Material"])
		{
			const std::string uuid = materialData.as<std::string>();

			AsyncAssetLoader::GetInstance().AsyncLoadMaterial(Utils::FindFilepath(uuid), [wEntity = std::weak_ptr(entity)](std::shared_ptr<Material> material)
			{
				if (std::shared_ptr<Entity> entity = wEntity.lock())
				{
					if (entity->HasComponent<Renderer3D>())
					{
						entity->GetComponent<Renderer3D>().material = material;
					}
				}
			});
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
	out << YAML::Key << "Ambient" << YAML::Value << directionalLight.ambient;

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

		if (const auto& ambientData = directionalLightData["Ambient"])
		{
			directionalLight.ambient = ambientData.as<float>();
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

	Logger::Log("Scene:" + filepath.string() + " has been serialized!", BOLDGREEN);
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

	Logger::Log("Scene:" + filepath.string() + " has been deserialized!", BOLDGREEN);

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
	out << YAML::Key << "ResolutionScale" << YAML::Value << graphicsSettings.ssao.resolutionScale;
	out << YAML::Key << "ResolutionBlurScale" << YAML::Value << graphicsSettings.ssao.resolutionBlurScale;

	out << YAML::EndMap;
	//

	// CSM.
	out << YAML::Key << "CSM";
	out << YAML::Value << YAML::BeginMap;

	out << YAML::Key << "IsEnabled" << YAML::Value << graphicsSettings.shadows.isEnabled;
	out << YAML::Key << "Quality" << YAML::Value << graphicsSettings.shadows.quality;
	out << YAML::Key << "CascadeCount" << YAML::Value << graphicsSettings.shadows.cascadeCount;
	out << YAML::Key << "SplitFactor" << YAML::Value << graphicsSettings.shadows.splitFactor;
	out << YAML::Key << "MaxDistance" << YAML::Value << graphicsSettings.shadows.maxDistance;
	out << YAML::Key << "FogFactor" << YAML::Value << graphicsSettings.shadows.fogFactor;
	out << YAML::Key << "PcfEnabled" << YAML::Value << graphicsSettings.shadows.pcfEnabled;
	out << YAML::Key << "PcfRange" << YAML::Value << graphicsSettings.shadows.pcfRange;
	out << YAML::Key << "Biases" << YAML::Value << graphicsSettings.shadows.biases;

	out << YAML::EndMap;
	//

	// Bloom.
	out << YAML::Key << "Bloom";
	out << YAML::Value << YAML::BeginMap;

	out << YAML::Key << "IsEnabled" << YAML::Value << graphicsSettings.bloom.isEnabled;
	out << YAML::Key << "MipCount" << YAML::Value << graphicsSettings.bloom.mipCount;
	out << YAML::Key << "BrightnessThreshold" << YAML::Value << graphicsSettings.bloom.brightnessThreshold;

	out << YAML::EndMap;
	//

	// SSR.
	out << YAML::Key << "SSR";
	out << YAML::Value << YAML::BeginMap;

	out << YAML::Key << "IsEnabled" << YAML::Value << graphicsSettings.ssr.isEnabled;
	out << YAML::Key << "IsMipMapsEnabled" << YAML::Value << graphicsSettings.ssr.isMipMapsEnabled;
	out << YAML::Key << "MaxDistance" << YAML::Value << graphicsSettings.ssr.maxDistance;
	out << YAML::Key << "Resolution" << YAML::Value << graphicsSettings.ssr.resolution;
	out << YAML::Key << "ResolutionBlurScale" << YAML::Value << graphicsSettings.ssr.resolutionBlurScale;
	out << YAML::Key << "ResolutionScale" << YAML::Value << graphicsSettings.ssr.resolutionScale;
	out << YAML::Key << "StepCount" << YAML::Value << graphicsSettings.ssr.stepCount;
	out << YAML::Key << "Thickness" << YAML::Value << graphicsSettings.ssr.thickness;
	out << YAML::Key << "BlurRange" << YAML::Value << graphicsSettings.ssr.blurRange;
	out << YAML::Key << "BlurOffset" << YAML::Value << graphicsSettings.ssr.blurOffset;

	out << YAML::EndMap;
	//

	// Post Process.
	out << YAML::Key << "PostProcess";
	out << YAML::Value << YAML::BeginMap;

	out << YAML::Key << "Gamma" << YAML::Value << graphicsSettings.postProcess.gamma;
	out << YAML::Key << "ToneMapper" << YAML::Value << (int)graphicsSettings.postProcess.toneMapper;
	out << YAML::Key << "FXAA" << YAML::Value << graphicsSettings.postProcess.fxaa;

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

		if (const auto& resolutionScaleData = ssaoData["ResolutionScale"])
		{
			graphicsSettings.ssao.resolutionScale = glm::min(resolutionScaleData.as<int>(), 3);
		}

		if (const auto& resolutionBlurScaleData = ssaoData["ResolutionBlurScale"])
		{
			graphicsSettings.ssao.resolutionBlurScale = glm::min(resolutionBlurScaleData.as<int>(), 3);
		}
	}

	if (const auto& csmData = data["CSM"])
	{
		if (const auto& isEnabledData = csmData["IsEnabled"])
		{
			graphicsSettings.shadows.isEnabled = isEnabledData.as<bool>();
		}

		if (const auto& qualityData = csmData["Quality"])
		{
			constexpr int maxQualityIndex = 2;
			graphicsSettings.shadows.quality = std::min(qualityData.as<int>(), maxQualityIndex);
		}

		if (const auto& cascadeCountData = csmData["CascadeCount"])
		{
			graphicsSettings.shadows.cascadeCount = cascadeCountData.as<int>();
		}

		if (const auto& splitFactorData = csmData["SplitFactor"])
		{
			graphicsSettings.shadows.splitFactor = splitFactorData.as<float>();
		}

		if (const auto& maxDistanceData = csmData["MaxDistance"])
		{
			graphicsSettings.shadows.maxDistance = maxDistanceData.as<float>();
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

	if (const auto& bloomData = data["Bloom"])
	{
		if (const auto& isEnabledData = bloomData["IsEnabled"])
		{
			graphicsSettings.bloom.isEnabled = isEnabledData.as<bool>();
		}

		if (const auto& mipCountData = bloomData["MipCount"])
		{
			graphicsSettings.bloom.mipCount = mipCountData.as<int>();
		}
	}

	if (const auto& ssrData = data["SSR"])
	{
		if (const auto& isEnabledData = ssrData["IsEnabled"])
		{
			graphicsSettings.ssr.isEnabled = isEnabledData.as<bool>();
		}

		if (const auto& isMipMapsEnabledData = ssrData["IsMipMapsEnabled"])
		{
			graphicsSettings.ssr.isMipMapsEnabled = isMipMapsEnabledData.as<bool>();
		}

		if (const auto& maxDistanceData = ssrData["MaxDistance"])
		{
			graphicsSettings.ssr.maxDistance = maxDistanceData.as<float>();
		}

		if (const auto& resolutionData = ssrData["Resolution"])
		{
			graphicsSettings.ssr.resolution = resolutionData.as<float>();
		}

		if (const auto& resolutionBlurScaleData = ssrData["ResolutionBlurScale"])
		{
			graphicsSettings.ssr.resolutionBlurScale = glm::min(resolutionBlurScaleData.as<int>(), 4);
		}

		if (const auto& resolutionScaleData = ssrData["ResolutionScale"])
		{
			graphicsSettings.ssr.resolutionScale = glm::min(resolutionScaleData.as<int>(), 3);
		}

		if (const auto& stepCountData = ssrData["StepCount"])
		{
			graphicsSettings.ssr.stepCount = stepCountData.as<int>();
		}

		if (const auto& thicknessData = ssrData["Thickness"])
		{
			graphicsSettings.ssr.thickness = thicknessData.as<float>();
		}

		if (const auto& blurRangeData = ssrData["BlurRange"])
		{
			graphicsSettings.ssr.blurRange = blurRangeData.as<int>();
		}

		if (const auto& blurOffsetData = ssrData["BlurOffset"])
		{
			graphicsSettings.ssr.blurOffset = blurOffsetData.as<int>();
		}
	}

	if (const auto& postProcessData = data["PostProcess"])
	{
		if (const auto& gammaData = postProcessData["Gamma"])
		{
			graphicsSettings.postProcess.gamma = gammaData.as<float>();
		}

		if (const auto& toneMapperData = postProcessData["ToneMapper"])
		{
			graphicsSettings.postProcess.toneMapper = (GraphicsSettings::PostProcess::ToneMapper)toneMapperData.as<int>();
		}

		if (const auto& fxaaData = postProcessData["FXAA"])
		{
			graphicsSettings.postProcess.fxaa = fxaaData.as<bool>();
		}
	}

	return graphicsSettings;
}

void Serializer::SerializeTextureMeta(const Texture::Meta& meta)
{
	if (meta.filepath.empty())
	{
		Logger::Error("Failed to serialize texture meta, filepath is empty!");
	}

	YAML::Emitter out;

	out << YAML::BeginMap;

	out << YAML::Key << "UUID" << YAML::Value << meta.uuid;
	out << YAML::Key << "SRGB" << YAML::Value << meta.srgb;
	out << YAML::Key << "CreateMipMaps" << YAML::Value << meta.createMipMaps;

	out << YAML::EndMap;

	std::ofstream fout(meta.filepath);
	fout << out.c_str();
	fout.close();
}

Texture::Meta Serializer::DeserializeTextureMeta(const std::filesystem::path& filepath)
{
	if (filepath.empty())
	{
		Logger::Error("Failed to serialize texture meta, filepath is empty!");
	}

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

	Texture::Meta meta{};
	if (YAML::Node uuidData = data["UUID"])
	{
		meta.uuid = uuidData.as<std::string>();
	}

	if (YAML::Node srgbData = data["SRGB"])
	{
		meta.srgb = srgbData.as<bool>();
	}

	if (YAML::Node createMipMapsData = data["CreateMipMaps"])
	{
		meta.createMipMaps = createMipMapsData.as<bool>();
	}

	meta.filepath = filepath;

	return meta;
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

					uniformsInfo.renderTargetsByName[uniformName] = renderTargetInfo;
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

		if (const auto& nameData = uniformData["RenderTargetDefault"])
		{
			uniformsInfo.renderTargetsByName[uniformName].renderTargetDefault = nameData.as<std::string>();
		}
	}
}

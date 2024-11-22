#include "VulkanPipeline.h"

#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanTexture.h"
#include "VulkanUniformLayout.h"
#include "VulkanUniformWriter.h"
#include "VulkanFormat.h"
#include "ShaderIncluder.h"

#include "../Core/Logger.h"
#include "../Core/Serializer.h"
#include "../Utils/Utils.h"

using namespace Pengine;
using namespace Vk;

VulkanPipeline::VulkanPipeline(const CreateInfo& pipelineCreateInfo)
	: Pipeline(pipelineCreateInfo)
{
	std::map<ShaderType, VkShaderModule> shaderModulesByType;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.reserve(pipelineCreateInfo.shaderFilepathsByType.size());

	shaderc::CompileOptions options{};
	options.SetOptimizationLevel(shaderc_optimization_level_performance);
	options.SetIncluder(std::make_unique<ShaderIncluder>());

	for (const auto& [type, filepath] : pipelineCreateInfo.shaderFilepathsByType)
	{
		const std::string vertexSpv = CompileShaderModule(filepath, options, type);
		m_ReflectShaderModulesByType[type] = Reflect(filepath, type);
		VkShaderModule shaderModule{};
		CreateShaderModule(vertexSpv, &shaderModule);
		shaderModulesByType[type] = shaderModule;
		CreateDescriptorSetLayouts(m_ReflectShaderModulesByType[type]);

		VkPipelineShaderStageCreateInfo& shaderStage = shaderStages.emplace_back();

		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = ConvertShaderStage(type);
		shaderStage.module = shaderModule;
		shaderStage.pName = "main";
		shaderStage.flags = 0;
		shaderStage.pNext = nullptr;
		shaderStage.pSpecializationInfo = nullptr;
	}

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	for (const auto& [set, uniformLayout] : m_UniformLayoutsByDescriptorSet)
	{
		descriptorSetLayouts.emplace_back(std::static_pointer_cast<VulkanUniformLayout>(uniformLayout)->GetDescriptorSetLayout());
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutCreateInfo,
		nullptr, &m_PipelineLayout) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create pipeline layout!");
	}

	PipelineConfigInfo pipelineConfigInfo{};
	DefaultPipelineConfigInfo(pipelineConfigInfo);

	pipelineConfigInfo.renderPass = std::static_pointer_cast<VulkanRenderPass>(
		pipelineCreateInfo.renderPass)->GetRenderPass();
	pipelineConfigInfo.pipelineLayout = m_PipelineLayout;
	pipelineConfigInfo.depthStencilInfo.depthWriteEnable = static_cast<VkBool32>(pipelineCreateInfo.depthWrite);
	pipelineConfigInfo.depthStencilInfo.depthTestEnable = static_cast<VkBool32>(pipelineCreateInfo.depthTest);
	pipelineConfigInfo.depthStencilInfo.depthCompareOp = static_cast<VkCompareOp>(pipelineCreateInfo.depthCompare);
	pipelineConfigInfo.depthStencilInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	pipelineConfigInfo.rasterizationInfo.cullMode = ConvertCullMode(pipelineCreateInfo.cullMode);
	pipelineConfigInfo.inputAssemblyInfo.topology = ConvertTopologyMode(pipelineCreateInfo.topologyMode);
	pipelineConfigInfo.rasterizationInfo.polygonMode = ConvertPolygonMode(pipelineCreateInfo.polygonMode);
	pipelineConfigInfo.colorBlendAttachments.clear();
	for (const auto& blendStateAttachment : pipelineCreateInfo.colorBlendStateAttachments)
	{
		VkPipelineColorBlendAttachmentState colorBlendState{};
		colorBlendState.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendState.blendEnable = blendStateAttachment.blendEnabled;
		colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
		pipelineConfigInfo.colorBlendAttachments.emplace_back(colorBlendState);
	}
	pipelineConfigInfo.colorBlendInfo.attachmentCount = pipelineConfigInfo.colorBlendAttachments.size();
	pipelineConfigInfo.colorBlendInfo.pAttachments = pipelineConfigInfo.colorBlendAttachments.data();

	auto bindingDescriptions = CreateBindingDescriptions(m_ReflectShaderModulesByType[ShaderType::VERTEX], pipelineCreateInfo.bindingDescriptions);
	auto attributeDescriptions = CreateAttributeDescriptions(m_ReflectShaderModulesByType[ShaderType::VERTEX], pipelineCreateInfo.bindingDescriptions);
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputCreateInfo.pVertexBindingDescriptions = bindingDescriptions.data();

	VkGraphicsPipelineCreateInfo vkPipelineCreateInfo{};
	vkPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	vkPipelineCreateInfo.stageCount = shaderStages.size();
	vkPipelineCreateInfo.pStages = shaderStages.data();
	vkPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	vkPipelineCreateInfo.pInputAssemblyState = &pipelineConfigInfo.inputAssemblyInfo;
	vkPipelineCreateInfo.pViewportState = &pipelineConfigInfo.viewportInfo;
	vkPipelineCreateInfo.pRasterizationState = &pipelineConfigInfo.rasterizationInfo;
	vkPipelineCreateInfo.pMultisampleState = &pipelineConfigInfo.multisampleInfo;
	vkPipelineCreateInfo.pColorBlendState = &pipelineConfigInfo.colorBlendInfo;
	vkPipelineCreateInfo.pDepthStencilState = &pipelineConfigInfo.depthStencilInfo;
	vkPipelineCreateInfo.pDynamicState = &pipelineConfigInfo.dynamicStateInfo;

	vkPipelineCreateInfo.layout = pipelineConfigInfo.pipelineLayout;
	vkPipelineCreateInfo.renderPass = pipelineConfigInfo.renderPass;
	vkPipelineCreateInfo.subpass = pipelineConfigInfo.subpass;

	vkPipelineCreateInfo.basePipelineIndex = -1;
	vkPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1,
		&vkPipelineCreateInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create graphics pipeline!");
	}

	for (auto& [type, shaderModule] : shaderModulesByType)
	{
		vkDestroyShaderModule(device->GetDevice(), shaderModule, nullptr);
	}
}

VulkanPipeline::~VulkanPipeline()
{
	device->DeleteResource([pipelineLayout = m_PipelineLayout, graphicsPipeline = m_GraphicsPipeline]()
	{
		vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);
		vkDestroyPipeline(device->GetDevice(), graphicsPipeline, nullptr);
	});
}

VkCullModeFlagBits VulkanPipeline::ConvertCullMode(const CullMode cullMode)
{
	switch (cullMode)
	{
	case Pengine::Pipeline::CullMode::NONE:
		return VK_CULL_MODE_NONE;
	case Pengine::Pipeline::CullMode::FRONT:
		return VK_CULL_MODE_FRONT_BIT;
	case Pengine::Pipeline::CullMode::BACK:
		return VK_CULL_MODE_BACK_BIT;
	case Pengine::Pipeline::CullMode::FRONT_AND_BACK:
		return VK_CULL_MODE_FRONT_AND_BACK;
	}

	FATAL_ERROR("Failed to convert cull mode!");
	return VkCullModeFlagBits::VK_CULL_MODE_NONE;
}

Pipeline::CullMode VulkanPipeline::ConvertCullMode(const VkCullModeFlagBits cullMode)
{
	switch (cullMode)
	{
	case VK_CULL_MODE_NONE:
		return Pengine::Pipeline::CullMode::NONE;
	case VK_CULL_MODE_FRONT_BIT:
		return Pengine::Pipeline::CullMode::FRONT;
	case VK_CULL_MODE_BACK_BIT:
		return Pengine::Pipeline::CullMode::BACK;
	case VK_CULL_MODE_FRONT_AND_BACK:
		return Pengine::Pipeline::CullMode::FRONT_AND_BACK;
	}

	FATAL_ERROR("Failed to convert cull mode!");
	return CullMode::NONE;
}

VkPrimitiveTopology VulkanPipeline::ConvertTopologyMode(TopologyMode topologyMode)
{
	switch (topologyMode)
	{
	case Pengine::Pipeline::TopologyMode::POINT_LIST:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case Pengine::Pipeline::TopologyMode::LINE_LIST:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case Pengine::Pipeline::TopologyMode::TRIANGLE_LIST:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	FATAL_ERROR("Failed to convert polygon mode!");
	return {};
}

Pipeline::TopologyMode VulkanPipeline::ConvertTopologyMode(VkPrimitiveTopology topologyMode)
{
	switch (topologyMode)
	{
	case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
		return Pengine::Pipeline::TopologyMode::POINT_LIST;
	case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		return Pengine::Pipeline::TopologyMode::LINE_LIST;
	case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		return Pengine::Pipeline::TopologyMode::TRIANGLE_LIST;
	}

	FATAL_ERROR("Failed to convert polygon mode!");
	return {};
}

VkPolygonMode VulkanPipeline::ConvertPolygonMode(const PolygonMode polygonMode)
{
	switch (polygonMode)
	{
	case Pengine::Pipeline::PolygonMode::FILL:
		return VK_POLYGON_MODE_FILL;
	case Pengine::Pipeline::PolygonMode::LINE:
		return VK_POLYGON_MODE_LINE;
	}

	FATAL_ERROR("Failed to convert polygon mode!");
	return {};
}

Pipeline::PolygonMode VulkanPipeline::ConvertPolygonMode(const VkPolygonMode polygonMode)
{
	switch (polygonMode)
	{
	case VK_POLYGON_MODE_FILL:
		return Pengine::Pipeline::PolygonMode::FILL;
	case VK_POLYGON_MODE_LINE:
		return Pengine::Pipeline::PolygonMode::LINE;
	}

	FATAL_ERROR("Failed to convert polygon mode!");
	return {};
}

VkShaderStageFlagBits VulkanPipeline::ConvertShaderStage(ShaderType stage)
{
	switch (stage)
	{
	case Pengine::Pipeline::ShaderType::VERTEX:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case Pengine::Pipeline::ShaderType::FRAGMENT:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case Pengine::Pipeline::ShaderType::GEOMETRY:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case Pengine::Pipeline::ShaderType::COMPUTE:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	}

	FATAL_ERROR("Failed to convert shader type!");
	return {};
}

Pipeline::ShaderType VulkanPipeline::ConvertShaderStage(VkShaderStageFlagBits stage)
{
	switch (stage)
	{
	case VK_SHADER_STAGE_VERTEX_BIT:
		return Pengine::Pipeline::ShaderType::VERTEX;
	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return Pengine::Pipeline::ShaderType::FRAGMENT;
	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return Pengine::Pipeline::ShaderType::GEOMETRY;
	case VK_SHADER_STAGE_COMPUTE_BIT:
		return Pengine::Pipeline::ShaderType::COMPUTE;
	}

	FATAL_ERROR("Failed to convert shader type!");
	return {};
}

std::vector<VkVertexInputBindingDescription> VulkanPipeline::CreateBindingDescriptions(
	const ShaderReflection::ReflectShaderModule& reflectShaderModule,
	const std::vector<BindingDescription>& bindingDescriptions)
{
	std::vector<VkVertexInputBindingDescription> vkBindingDescriptions;
	vkBindingDescriptions.reserve(bindingDescriptions.size());

	for (const BindingDescription& bindingDescription : bindingDescriptions)
	{
		VkVertexInputBindingDescription& vkBindingDescription = vkBindingDescriptions.emplace_back();

		vkBindingDescription.binding = bindingDescription.binding;
		vkBindingDescription.inputRate = ConvertVertexInputRate(bindingDescription.inputRate);

		uint32_t stride = 0;
		for (const std::string& name : bindingDescription.names)
		{
			// Looking for an attribute description with a specific name.
			std::optional<ShaderReflection::AttributeDescription> foundAttributeDescription;
			for (const auto& attributeDescription : reflectShaderModule.attributeDescriptions)
			{
				if (attributeDescription.name == name)
				{
					foundAttributeDescription = attributeDescription;
					break;
				}
			}

			if (foundAttributeDescription)
			{
				stride += foundAttributeDescription->count * foundAttributeDescription->size;
			}
			else
			{
				FATAL_ERROR("Failed to find vertex input attribute with the name: " + name);
			}
		}

		vkBindingDescription.stride = stride;
	}

	return vkBindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> VulkanPipeline::CreateAttributeDescriptions(
	const ShaderReflection::ReflectShaderModule& reflectShaderModule,
	const std::vector<BindingDescription>& bindingDescriptions)
{
	std::vector<VkVertexInputAttributeDescription> vkAttributeDescriptions;

	std::map<uint32_t, std::vector<ShaderReflection::AttributeDescription>> sortedAttributeDescriptionsByBinding;

	for (const BindingDescription& bindingDescription : bindingDescriptions)
	{
		for (const std::string& name : bindingDescription.names)
		{
			// Looking for an attribute description with a specific name.
			std::optional<ShaderReflection::AttributeDescription> foundAttributeDescription;
			for (const auto& attributeDescription : reflectShaderModule.attributeDescriptions)
			{
				if (attributeDescription.name == name)
				{
					foundAttributeDescription = attributeDescription;
					break;
				}
			}

			if (foundAttributeDescription)
			{
				sortedAttributeDescriptionsByBinding[bindingDescription.binding].emplace_back(*foundAttributeDescription);
			}
			else
			{
				FATAL_ERROR("Failed to find vertex input attribute with the name: " + name);
			}
		}
	}

	for (const auto& [binding, attributeDescriptions] : sortedAttributeDescriptionsByBinding)
	{
		uint32_t offset = 0;
		for (const auto& attributeDescription : attributeDescriptions)
		{
			for (size_t row = 0; row < attributeDescription.count; row++)
			{
				VkVertexInputAttributeDescription& vkAttributeDescription = vkAttributeDescriptions.emplace_back();
				vkAttributeDescription.binding = binding;
				vkAttributeDescription.format = ConvertFormat(attributeDescription.format);
				vkAttributeDescription.location = attributeDescription.location + row;
				vkAttributeDescription.offset = offset;

				offset += attributeDescription.size;
			}
		}
	}

	return vkAttributeDescriptions;
}

VkVertexInputRate VulkanPipeline::ConvertVertexInputRate(const InputRate vertexInputRate)
{
	switch (vertexInputRate)
	{
	case Pengine::Vk::VulkanPipeline::InputRate::VERTEX:
		return VK_VERTEX_INPUT_RATE_VERTEX;
	case Pengine::Vk::VulkanPipeline::InputRate::INSTANCE:
		return VK_VERTEX_INPUT_RATE_INSTANCE;
	}

	FATAL_ERROR("Failed to convert vertex input rate!");
	return VkVertexInputRate::VK_VERTEX_INPUT_RATE_MAX_ENUM;
}

VulkanPipeline::InputRate VulkanPipeline::ConvertVertexInputRate(const VkVertexInputRate vertexInputRate)
{
	switch (vertexInputRate)
	{
	case VK_VERTEX_INPUT_RATE_VERTEX:
		return Pengine::Vk::VulkanPipeline::InputRate::VERTEX;
	case VK_VERTEX_INPUT_RATE_INSTANCE:
		return Pengine::Vk::VulkanPipeline::InputRate::INSTANCE;
	}

	FATAL_ERROR("Failed to convert vertex input rate!");
	return {};
}

void VulkanPipeline::CreateShaderModule(
	const std::string& code,
	VkShaderModule* shaderModule)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	if (vkCreateShaderModule(device->GetDevice(), &shaderModuleCreateInfo, nullptr,
		shaderModule) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create shader module!");
	}
}

std::string VulkanPipeline::CompileShaderModule(
	const std::string& filepath,
	shaderc::CompileOptions options,
	const ShaderType type,
	bool useCache,
	bool useLog)
{
	shaderc_shader_kind kind;
	switch (type)
	{
	case Pengine::Pipeline::ShaderType::VERTEX:
		kind = shaderc_shader_kind::shaderc_glsl_vertex_shader;
		break;
	case Pengine::Pipeline::ShaderType::FRAGMENT:
		kind = shaderc_shader_kind::shaderc_glsl_fragment_shader;
		break;
	case Pengine::Pipeline::ShaderType::GEOMETRY:
		kind = shaderc_shader_kind::shaderc_glsl_geometry_shader;
		break;
	case Pengine::Pipeline::ShaderType::COMPUTE:
		kind = shaderc_shader_kind::shaderc_glsl_compute_shader;
		break;
	default:
		return {};
	}

	std::string spv;
	
	if (useCache)
	{
		spv = Serializer::DeserializeShaderCache(filepath);
	}

	if (spv.empty())
	{
		shaderc::Compiler compiler{};

		shaderc::SpvCompilationResult module =
			compiler.CompileGlslToSpv(Utils::ReadFile(filepath), kind, filepath.c_str(), options);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			Logger::Error(module.GetErrorMessage());
			return {};
		}

		spv = std::move(std::string((const char*)module.cbegin(), (const char*)module.cend()));

		if (useCache)
		{
			Serializer::SerializeShaderCache(filepath, spv);
		}

		if (useLog)
		{
			Logger::Log("Shader:" + filepath + " has been compiled!", GREEN);
		}
	}
	else
	{
		if (useLog)
		{
			Logger::Log("Shader Cache:" + filepath + " has been loaded!", GREEN);
		}
	}

	return spv;
}

ShaderReflection::ReflectShaderModule VulkanPipeline::Reflect(const std::string& filepath, ShaderType type)
{
	std::optional<ShaderReflection::ReflectShaderModule> loadedReflectShaderModule = Serializer::DeserializeShaderModuleReflection(filepath);
	if (loadedReflectShaderModule)
	{
		return *loadedReflectShaderModule;
	}

	shaderc::CompileOptions options{};
	options.SetOptimizationLevel(shaderc_optimization_level_zero);
	options.SetGenerateDebugInfo();
	options.SetPreserveBindings(true);
	options.SetIncluder(std::make_unique<ShaderIncluder>());

	std::string spv = CompileShaderModule(filepath, options, type, false, false);

	SpvReflectShaderModule reflectModule{};
	SpvReflectResult result = spvReflectCreateShaderModule(spv.size(), spv.data(), &reflectModule);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		FATAL_ERROR("Failed to get spirv reflection!");
	}

	ShaderReflection::ReflectShaderModule reflectShaderModule{};

	ReflectDescriptorSets(reflectModule, reflectShaderModule);
	ReflectInputVariables(reflectModule, reflectShaderModule);

	spvReflectDestroyShaderModule(&reflectModule);

	Serializer::SerializeShaderModuleReflection(filepath, reflectShaderModule);

	return reflectShaderModule;
}

void VulkanPipeline::ReflectDescriptorSets(
	SpvReflectShaderModule& reflectModule,
	ShaderReflection::ReflectShaderModule& reflectShaderModule)
{
	uint32_t count = 0;
	SpvReflectResult result = spvReflectEnumerateDescriptorSets(&reflectModule, &count, NULL);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		FATAL_ERROR("Failed to reflect to enumerate descriptor sets!");
	}

	std::vector<SpvReflectDescriptorSet*> reflectSets(count);
	result = spvReflectEnumerateDescriptorSets(&reflectModule, &count, reflectSets.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		FATAL_ERROR("Failed to reflect to enumerate descriptor sets!");
	}

	std::function<void(ShaderReflection::ReflectVariable&, const SpvReflectBlockVariable&)> reflectStruct;

	reflectStruct = [&reflectStruct](
		ShaderReflection::ReflectVariable& variable,
		const SpvReflectBlockVariable& reflectBlock)
		{
			for (uint32_t memberIndex = 0; memberIndex < reflectBlock.member_count; memberIndex++)
			{
				const auto& member = reflectBlock.members[memberIndex];
				ShaderReflection::ReflectVariable& memberVariable = variable.variables.emplace_back();

				memberVariable.name = member.name;
				memberVariable.size = member.size;
				memberVariable.offset = member.offset;
				memberVariable.type = ShaderReflection::ReflectVariable::Type::UNDEFINED;

				// NOTE: Support only single dimensional array.
				if (member.array.dims_count == 1)
				{
					memberVariable.count = member.array.dims[0];
				}

				if (!member.type_description)
				{
					continue;
				}

				switch (member.type_description->op)
				{
				case SpvOp::SpvOpTypeStruct:
				{
					memberVariable.type = ShaderReflection::ReflectVariable::Type::STRUCT;
					reflectStruct(memberVariable, member);
					break;
				}
				case SpvOp::SpvOpTypeMatrix:
				{
					memberVariable.type = ShaderReflection::ReflectVariable::Type::MATRIX;
					break;
				}
				case SpvOp::SpvOpTypeFloat:
				{
					memberVariable.type = ShaderReflection::ReflectVariable::Type::FLOAT;
					break;
				}
				case SpvOp::SpvOpTypeInt:
				{
					memberVariable.type = ShaderReflection::ReflectVariable::Type::INT;
					break;
				}
				case SpvOp::SpvOpTypeVector:
				{
					switch (member.numeric.vector.component_count)
					{
					case 2:
					{
						memberVariable.type = ShaderReflection::ReflectVariable::Type::VEC2;
						break;
					}
					case 3:
					{
						memberVariable.type = ShaderReflection::ReflectVariable::Type::VEC3;
						break;
					}
					case 4:
					{
						memberVariable.type = ShaderReflection::ReflectVariable::Type::VEC4;
						break;
					}
					default:
						break;
					}
					break;
				}
				default:
					break;
				}

				if (member.member_count > 0 && member.array.dims_count == 1)
				{
					memberVariable.type = ShaderReflection::ReflectVariable::Type::STRUCT;
					reflectStruct(memberVariable, member);
				}
			}
		};

	for (const auto& reflectSet : reflectSets)
	{
		ShaderReflection::ReflectDescriptorSetLayout& setLayout = reflectShaderModule.setLayouts.emplace_back();
		setLayout.set = reflectSet->set;
		for (uint32_t bindingIndex = 0; bindingIndex < reflectSet->binding_count; ++bindingIndex)
		{
			const SpvReflectDescriptorBinding& reflectBinding = *(reflectSet->bindings[bindingIndex]);
			ShaderReflection::ReflectDescriptorSetBinding& binding = setLayout.bindings.emplace_back();

			if (reflectBinding.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			{
				binding.name = reflectBinding.type_description->type_name;
				binding.binding = reflectBinding.binding;
				binding.type = VulkanUniformLayout::ConvertDescriptorType(static_cast<VkDescriptorType>(reflectBinding.descriptor_type));
				binding.count = 1;

				ShaderReflection::ReflectVariable& buffer = binding.buffer.emplace();
				buffer.size = reflectBinding.block.size;
				buffer.name = reflectBinding.type_description->type_name;
				buffer.offset = reflectBinding.block.offset;
				buffer.type = ShaderReflection::ReflectVariable::Type::STRUCT;

				reflectStruct(buffer, reflectBinding.block);
			}
			else if (reflectBinding.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				binding.name = reflectBinding.name;
				binding.binding = reflectBinding.binding;
				binding.type = VulkanUniformLayout::ConvertDescriptorType(static_cast<VkDescriptorType>(reflectBinding.descriptor_type));
				binding.count = 1;
				for (uint32_t dimIndex = 0; dimIndex < reflectBinding.array.dims_count; ++dimIndex)
				{
					// Check exactly.
					binding.count *= reflectBinding.array.dims[dimIndex];
				}
			}
		}
	}
}

void VulkanPipeline::ReflectInputVariables(
	SpvReflectShaderModule& reflectModule,
	ShaderReflection::ReflectShaderModule& reflectShaderModule)
{
	uint32_t count = 0;
	SpvReflectResult result = spvReflectEnumerateInputVariables(&reflectModule, &count, NULL);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		FATAL_ERROR("Failed to reflect to enumerate descriptor sets!");
	}

	std::vector<SpvReflectInterfaceVariable*> inputVariables(count);
	result = spvReflectEnumerateInputVariables(&reflectModule, &count, inputVariables.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		FATAL_ERROR("Failed to reflect to enumerate descriptor sets!");
	}

	for (const auto& inputVariable : inputVariables)
	{
		if (inputVariable->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
		{
			continue;
		}

		ShaderReflection::AttributeDescription& attributeDescription = reflectShaderModule.attributeDescriptions.emplace_back();
		attributeDescription.name = inputVariable->name;
		attributeDescription.location = inputVariable->location;
		attributeDescription.format = ConvertFormat(static_cast<VkFormat>(inputVariable->format));
		attributeDescription.size = FormatSize(attributeDescription.format);
		attributeDescription.count = 1;

		// Matrix as a shader vertex attribute is splited into a certain amount of vectors,
		// so we need to now how many of these vectors are there.
		if (inputVariable->type_description->op == SpvOp::SpvOpTypeMatrix)
		{
			attributeDescription.count = inputVariable->numeric.matrix.row_count;
		}
	}
}

void VulkanPipeline::CreateDescriptorSetLayouts(const ShaderReflection::ReflectShaderModule& reflectShaderModule)
{
	for (const auto& [set, bindings] : reflectShaderModule.setLayouts)
	{
		m_UniformLayoutsByDescriptorSet[set] = UniformLayout::Create(bindings);
	}
}

void VulkanPipeline::Bind(const VkCommandBuffer commandBuffer) const
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
}

void VulkanPipeline::DefaultPipelineConfigInfo(PipelineConfigInfo& pipelineConfigInfo)
{
	pipelineConfigInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineConfigInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineConfigInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	pipelineConfigInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineConfigInfo.viewportInfo.viewportCount = 1;
	pipelineConfigInfo.viewportInfo.pViewports = nullptr;
	pipelineConfigInfo.viewportInfo.scissorCount = 1;
	pipelineConfigInfo.viewportInfo.pScissors = nullptr;

	pipelineConfigInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineConfigInfo.rasterizationInfo.depthClampEnable = VK_TRUE;
	pipelineConfigInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	pipelineConfigInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineConfigInfo.rasterizationInfo.lineWidth = 1.0f;
	pipelineConfigInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	pipelineConfigInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipelineConfigInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
	pipelineConfigInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
	pipelineConfigInfo.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
	pipelineConfigInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional

	pipelineConfigInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineConfigInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
	pipelineConfigInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineConfigInfo.multisampleInfo.minSampleShading = 1.0f;           // Optional
	pipelineConfigInfo.multisampleInfo.pSampleMask = nullptr;             // Optional
	pipelineConfigInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
	pipelineConfigInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

	pipelineConfigInfo.colorBlendAttachments.resize(1);
	pipelineConfigInfo.colorBlendAttachments[0].colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	pipelineConfigInfo.colorBlendAttachments[0].blendEnable = VK_FALSE;
	pipelineConfigInfo.colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;   // Optional
	pipelineConfigInfo.colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;  // Optional
	pipelineConfigInfo.colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;              // Optional
	pipelineConfigInfo.colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
	pipelineConfigInfo.colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
	pipelineConfigInfo.colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

	pipelineConfigInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineConfigInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
	pipelineConfigInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
	pipelineConfigInfo.colorBlendInfo.attachmentCount = pipelineConfigInfo.colorBlendAttachments.size();
	pipelineConfigInfo.colorBlendInfo.pAttachments = pipelineConfigInfo.colorBlendAttachments.data();
	pipelineConfigInfo.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
	pipelineConfigInfo.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
	pipelineConfigInfo.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
	pipelineConfigInfo.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

	pipelineConfigInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineConfigInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
	pipelineConfigInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
	pipelineConfigInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	pipelineConfigInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	pipelineConfigInfo.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
	pipelineConfigInfo.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
	pipelineConfigInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
	pipelineConfigInfo.depthStencilInfo.front = {};  // Optional
	pipelineConfigInfo.depthStencilInfo.back = {};   // Optional

	pipelineConfigInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pipelineConfigInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineConfigInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(pipelineConfigInfo.dynamicStateEnables.size());
	pipelineConfigInfo.dynamicStateInfo.pDynamicStates = pipelineConfigInfo.dynamicStateEnables.data();
	pipelineConfigInfo.dynamicStateInfo.flags = 0;
}

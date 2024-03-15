#include "VulkanPipeline.h"

#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanTexture.h"
#include "VulkanUniformLayout.h"
#include "VulkanUniformWriter.h"

#include "../Core/Logger.h"
#include "../Core/Timer.h"
#include "../Core/Serializer.h"
#include "../Core/ViewportManager.h"
#include "../Graphics/Renderer.h"
#include "../Source/SPIRV-Reflect/spirv_reflect.h"

#include <shaderc/shaderc.hpp>

#include <glfw3.h>

using namespace Pengine;
using namespace Vk;

VulkanPipeline::VulkanPipeline(const CreateInfo& pipelineCreateInfo)
    : Pipeline(pipelineCreateInfo)
{
    std::vector<VkDescriptorSetLayout> layouts;
    if (pipelineCreateInfo.renderPass->GetUniformWriter())
    {
        layouts.emplace_back(std::static_pointer_cast<VulkanUniformLayout>(
            pipelineCreateInfo.renderPass->GetUniformWriter()->GetLayout())->GetDescriptorSetLayout());
    }

    if (m_UniformWriter)
    {
        layouts.emplace_back(std::static_pointer_cast<VulkanUniformLayout>(
            m_UniformWriter->GetLayout())->GetDescriptorSetLayout());
    }

    if (m_ChildUniformLayout)
    {
        layouts.emplace_back(std::static_pointer_cast<VulkanUniformLayout>(
            m_ChildUniformLayout)->GetDescriptorSetLayout());
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutCreateInfo,
        nullptr, &m_PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    PipelineConfigInfo pipelineConfigInfo{};
    DefaultPipelineConfigInfo(pipelineConfigInfo);
    pipelineConfigInfo.renderPass = std::static_pointer_cast<VulkanRenderPass>(
        pipelineCreateInfo.renderPass)->GetRenderPass();
    pipelineConfigInfo.pipelineLayout = m_PipelineLayout;
    pipelineConfigInfo.depthStencilInfo.depthWriteEnable = (VkBool32)pipelineCreateInfo.depthWrite;
    pipelineConfigInfo.depthStencilInfo.depthTestEnable = (VkBool32)pipelineCreateInfo.depthTest;
    pipelineConfigInfo.rasterizationInfo.cullMode = ConvertCullMode(pipelineCreateInfo.cullMode);
    pipelineConfigInfo.rasterizationInfo.polygonMode = ConvertPolygonMode(pipelineCreateInfo.polygonMode);
    pipelineConfigInfo.colorBlendAttachments.clear();
    for (const auto& blendStateAttachment : pipelineCreateInfo.colorBlendStateAttachments)
    {
        VkPipelineColorBlendAttachmentState colorBlendState{};
        colorBlendState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendState.blendEnable = blendStateAttachment.blendEnabled;
        colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendState.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
        pipelineConfigInfo.colorBlendAttachments.emplace_back(colorBlendState);
    }
    pipelineConfigInfo.colorBlendInfo.attachmentCount = pipelineConfigInfo.colorBlendAttachments.size();
    pipelineConfigInfo.colorBlendInfo.pAttachments = pipelineConfigInfo.colorBlendAttachments.data();

    std::string vertexSpv = CompileShaderModule(pipelineCreateInfo.vertexFilepath, ShaderType::VERTEX);
    CreateShaderModule(vertexSpv, &m_VertShaderModule);
    std::string fragmentSpv = CompileShaderModule(pipelineCreateInfo.fragmentFilepath, ShaderType::FRAGMENT);
    CreateShaderModule(fragmentSpv, &m_FragShaderModule);

    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = m_VertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = m_FragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;

    auto bindingDescriptions = CreateBindingDescriptions(pipelineCreateInfo.renderPass->GetBindingDescriptions());
    auto attributeDescriptions = CreateAttributeDescriptions(pipelineCreateInfo.renderPass->GetAttributeDescriptions());
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
    vertexInputCreateInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    vertexInputCreateInfo.pVertexBindingDescriptions = bindingDescriptions.data();

    VkGraphicsPipelineCreateInfo vkPipelineCreateInfo{};
    vkPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    vkPipelineCreateInfo.stageCount = 2;
    vkPipelineCreateInfo.pStages = &shaderStages[0];
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
        FATAL_ERROR("Failed to create graphics pipeline!")
    }
}

VulkanPipeline::~VulkanPipeline()
{
    vkDeviceWaitIdle(device->GetDevice());

    vkDestroyShaderModule(device->GetDevice(), m_VertShaderModule, nullptr);
    vkDestroyShaderModule(device->GetDevice(), m_FragShaderModule, nullptr);

    vkDestroyPipelineLayout(device->GetDevice(), m_PipelineLayout, nullptr);
    vkDestroyPipeline(device->GetDevice(), m_GraphicsPipeline, nullptr);
}

VkCullModeFlagBits VulkanPipeline::ConvertCullMode(CullMode cullMode)
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

    FATAL_ERROR("Failed to convert cull mode!")
}

Pipeline::CullMode VulkanPipeline::ConvertCullMode(VkCullModeFlagBits cullMode)
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

    FATAL_ERROR("Failed to convert cull mode!")
}

VkPolygonMode VulkanPipeline::ConvertPolygonMode(Pipeline::PolygonMode polygonMode)
{
    switch (polygonMode)
    {
    case Pengine::Pipeline::PolygonMode::FILL:
        return VK_POLYGON_MODE_FILL;
    case Pengine::Pipeline::PolygonMode::LINE:
        return VK_POLYGON_MODE_LINE;
    }

    FATAL_ERROR("Failed to convert polygon mode!")
}

Pipeline::PolygonMode VulkanPipeline::ConvertPolygonMode(VkPolygonMode polygonMode)
{
    switch (polygonMode)
    {
    case VK_POLYGON_MODE_FILL:
        return Pengine::Pipeline::PolygonMode::FILL;
    case VK_POLYGON_MODE_LINE:
        return Pengine::Pipeline::PolygonMode::LINE;
    }

    FATAL_ERROR("Failed to convert polygon mode!")
}

std::vector<VkVertexInputBindingDescription> VulkanPipeline::CreateBindingDescriptions(
    std::vector<Vertex::BindingDescription> bindingDescriptions)
{
    std::vector<VkVertexInputBindingDescription> vkBindingDescriptions(bindingDescriptions.size());
    for (size_t i = 0; i < bindingDescriptions.size(); i++)
    {
        vkBindingDescriptions[i].binding = bindingDescriptions[i].binding;
        vkBindingDescriptions[i].stride = bindingDescriptions[i].stride;
        vkBindingDescriptions[i].inputRate = ConvertVertexInputRate(bindingDescriptions[i].inputRate);
    }

    return vkBindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> VulkanPipeline::CreateAttributeDescriptions(
    std::vector<Vertex::AttributeDescription> attributeDescriptions)
{
    std::vector<VkVertexInputAttributeDescription> vkAttributeDescriptions(attributeDescriptions.size());
    for (size_t i = 0; i < attributeDescriptions.size(); i++)
    {
        vkAttributeDescriptions[i].binding = attributeDescriptions[i].binding;
        vkAttributeDescriptions[i].format = VulkanTexture::ConvertFormat(attributeDescriptions[i].format);
        vkAttributeDescriptions[i].location = attributeDescriptions[i].location;
        vkAttributeDescriptions[i].offset = attributeDescriptions[i].offset;
    }

    return vkAttributeDescriptions;
}

VkVertexInputRate VulkanPipeline::ConvertVertexInputRate(Vertex::InputRate vertexInputRate)
{
    switch (vertexInputRate)
    {
    case Pengine::Vertex::InputRate::VERTEX:
        return VK_VERTEX_INPUT_RATE_VERTEX;
    case Pengine::Vertex::InputRate::INSTANCE:
        return VK_VERTEX_INPUT_RATE_INSTANCE;
    }

    FATAL_ERROR("Failed to convert vertex input rate!")
}

Vertex::InputRate VulkanPipeline::ConvertVertexInputRate(VkVertexInputRate vertexInputRate)
{
    switch (vertexInputRate)
    {
    case VK_VERTEX_INPUT_RATE_VERTEX:
        return Pengine::Vertex::InputRate::VERTEX;
    case VK_VERTEX_INPUT_RATE_INSTANCE:
        return Pengine::Vertex::InputRate::INSTANCE;
    }

    FATAL_ERROR("Failed to convert vertex input rate!")
}

void VulkanPipeline::CreateShaderModule(const std::string& code,
    VkShaderModule* shaderModule)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = code.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(device->GetDevice(), &shaderModuleCreateInfo, nullptr,
        shaderModule) != VK_SUCCESS)
    {
        FATAL_ERROR("Failed to create shader module!")
    }
}

std::string VulkanPipeline::CompileShaderModule(const std::string& filepath, ShaderType type)
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
    default:
        return {};
    }

    std::string spv = Serializer::DeserializeShaderCache(filepath);
    if (spv.empty())
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetOptimizationLevel(shaderc_optimization_level_size);

        shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(ReadFile(filepath), kind, filepath.c_str(), options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            Logger::Error(module.GetErrorMessage());
            return {};
        }

        spv = std::move(std::string((const char*)module.cbegin(), (const char*)module.cend()));

        Serializer::SerializeShaderCache(filepath, spv);
    }
    
    Reflect(spv);

    return spv;
}

void VulkanPipeline::Reflect(const std::string& spv)
{
    // In progress.
    SpvReflectShaderModule module = {};
    SpvReflectResult result = spvReflectCreateShaderModule(spv.size(), spv.data(), &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        FATAL_ERROR("Failed to get spirv reflection");
    }

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorBindings(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    SpvReflectDescriptorBinding** input_vars =
        (SpvReflectDescriptorBinding**)malloc(count * sizeof(SpvReflectDescriptorBinding*));
    result = spvReflectEnumerateDescriptorBindings(&module, &count, input_vars);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    for (uint32_t i = 0; i < count; i++)
    {
        const auto var = input_vars[i];
    }

    spvReflectDestroyShaderModule(&module);
}

void VulkanPipeline::Bind(VkCommandBuffer commandBuffer)
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
    pipelineConfigInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
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
    pipelineConfigInfo.colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    pipelineConfigInfo.colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
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
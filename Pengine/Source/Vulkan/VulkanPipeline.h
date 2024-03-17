#pragma once

#include "../Core/Core.h"
#include "../Graphics/Pipeline.h"

#include <vulkan/vulkan.h>

namespace Pengine::Vk
{

    struct PipelineConfigInfo
    {
        PipelineConfigInfo(PipelineConfigInfo const&) = delete;
        PipelineConfigInfo& operator=(PipelineConfigInfo const&) = delete;

        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };

    class PENGINE_API VulkanPipeline final : public Pipeline
    {
    public:
        static void DefaultPipelineConfigInfo(PipelineConfigInfo& pipelineConfigInfo);

        void Bind(VkCommandBuffer commandBuffer) const;

        explicit VulkanPipeline(const CreateInfo& pipelineCreateInfo);
        virtual ~VulkanPipeline() override;
        VulkanPipeline(const VulkanPipeline&) = delete;
        VulkanPipeline& operator=(const VulkanPipeline&) = delete;

        static VkCullModeFlagBits ConvertCullMode(CullMode cullMode);

        static CullMode ConvertCullMode(VkCullModeFlagBits cullMode);

        static VkPolygonMode ConvertPolygonMode(PolygonMode polygonMode);

        static PolygonMode ConvertPolygonMode(VkPolygonMode polygonMode);

        VkPipeline GetPipeline() const { return m_GraphicsPipeline; }

        VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

    private:
        static std::vector<VkVertexInputBindingDescription> CreateBindingDescriptions(
            const std::vector<Vertex::BindingDescription>& bindingDescriptions);

        static std::vector<VkVertexInputAttributeDescription> CreateAttributeDescriptions(
            const std::vector<Vertex::AttributeDescription>& attributeDescriptions);

        static VkVertexInputRate ConvertVertexInputRate(Vertex::InputRate vertexInputRate);

        static Vertex::InputRate ConvertVertexInputRate(VkVertexInputRate vertexInputRate);

        static void CreateShaderModule(const std::string& code, VkShaderModule* shaderModule);

        std::string CompileShaderModule(const std::string& filepath, ShaderType type);

        static void Reflect(const std::string& spv);

        VkPipeline m_GraphicsPipeline{};
        VkPipelineLayout m_PipelineLayout{};
        VkShaderModule m_VertShaderModule{};
        VkShaderModule m_FragShaderModule{};
    };

}
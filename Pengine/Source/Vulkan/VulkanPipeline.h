#pragma once

#include "../Core/Core.h"
#include "../Graphics/Pipeline.h"

namespace Pengine
{

    namespace Vk
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

        class PENGINE_API VulkanPipeline : public Pipeline
        {
        public:
            static void DefaultPipelineConfigInfo(PipelineConfigInfo& pipelineConfigInfo);

            void Bind(VkCommandBuffer commandBuffer);

            VulkanPipeline(const CreateInfo& pipelineCreateInfo);
            virtual ~VulkanPipeline() override;
            VulkanPipeline(const VulkanPipeline&) = delete;
            VulkanPipeline& operator=(const VulkanPipeline&) = delete;

            VkCullModeFlagBits ConvertCullMode(CullMode cullMode);

            CullMode ConvertCullMode(VkCullModeFlagBits cullMode);

            VkPolygonMode ConvertPolygonMode(PolygonMode polygonMode);

            PolygonMode ConvertPolygonMode(VkPolygonMode polygonMode);

            VkPipeline GetPipeline() const { return m_GraphicsPipeline; }

            VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

        private:
            static std::vector<VkVertexInputBindingDescription> CreateBindingDescriptions(
                std::vector<Vertex::BindingDescription> bindingDescriptions);

            static std::vector<VkVertexInputAttributeDescription> CreateAttributeDescriptions(
                std::vector<Vertex::AttributeDescription> attributeDescriptions);
            
            static VkVertexInputRate ConvertVertexInputRate(Vertex::InputRate vertexInputRate);

            static Vertex::InputRate ConvertVertexInputRate(VkVertexInputRate vertexInputRate);

            void CreateShaderModule(const std::string& code, VkShaderModule* shaderModule);

            std::string CompileShaderModule(const std::string& filepath, ShaderType type);

            VkPipeline m_GraphicsPipeline;
            VkPipelineLayout m_PipelineLayout;
            VkShaderModule m_VertShaderModule;
            VkShaderModule m_FragShaderModule;
        };

    }

}
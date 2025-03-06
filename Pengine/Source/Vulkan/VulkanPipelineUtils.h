#pragma once

#include "../Core/Core.h"
#include "../Graphics/Pipeline.h"

#include <vulkan/vulkan.h>
#include <SPIRV-Reflect/spirv_reflect.h>

#include <shaderc/shaderc.hpp>

namespace Pengine
{

	class PENGINE_API VulkanPipelineUtils
	{
	public:
		static void CreateShaderModule(const std::string& code, VkShaderModule* shaderModule);

		static std::string CompileShaderModule(
			const std::string& filepath,
			shaderc::CompileOptions options,
			Pipeline::ShaderType type,
			bool useCache = true,
			bool useLog = true);

		static ShaderReflection::ReflectShaderModule Reflect(const std::string& filepath, Pipeline::ShaderType type);

		static void ReflectDescriptorSets(
			SpvReflectShaderModule& reflectModule,
			ShaderReflection::ReflectShaderModule& reflectShaderModule);

		static void ReflectInputVariables(
			SpvReflectShaderModule& reflectModule,
			ShaderReflection::ReflectShaderModule& reflectShaderModule);

		static std::map<uint32_t, std::shared_ptr<UniformLayout>> CreateDescriptorSetLayouts(const ShaderReflection::ReflectShaderModule& reflectShaderModule);

		static VkShaderStageFlagBits ConvertShaderStage(Pipeline::ShaderType stage);

		static Pipeline::ShaderType ConvertShaderStage(VkShaderStageFlagBits stage);
	};

}

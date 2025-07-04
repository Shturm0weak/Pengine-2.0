#pragma once

#include "../Core/Core.h"

#include "ShaderModule.h"

namespace Pengine
{

	class PENGINE_API ShaderModuleManager
	{
	public:
		static ShaderModuleManager& GetInstance();

		ShaderModuleManager(const ShaderModuleManager&) = delete;
		ShaderModuleManager& operator=(const ShaderModuleManager&) = delete;

		[[nodiscard]] std::shared_ptr<ShaderModule> GetOrCreateShaderModule(
			const std::filesystem::path& filepath,
			const ShaderModule::Type type);

		[[nodiscard]] std::shared_ptr<ShaderModule> CreateShaderModule(
			const std::filesystem::path& filepath,
			const ShaderModule::Type type);

		[[nodiscard]] std::shared_ptr<ShaderModule> GetShaderModule(const std::filesystem::path& filepath) const;

		void DeleteShaderModule(std::shared_ptr<ShaderModule>& shaderModule);

		void ShutDown();

	private:
		std::unordered_map<std::filesystem::path, std::shared_ptr<ShaderModule>> m_ShaderModulesByFilepath;

		mutable std::mutex m_ShaderModuleMutex;
		mutable std::mutex m_CreatingShaderModuleMutex;
		mutable std::condition_variable m_CreatingShaderModuleConditionVariable{};
		std::set<std::filesystem::path> m_CreatingShaderModules;

		ShaderModuleManager() = default;
		~ShaderModuleManager() = default;
	};

}

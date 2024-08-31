#pragma once

#include "../Core/Core.h"

#include "ShaderReflection.h"

namespace Pengine
{

	class PENGINE_API UniformLayout
	{
	public:
		struct RenderTargetInfo
		{
			std::string renderPassName;
			uint32_t attachmentIndex;
		};

		static std::shared_ptr<UniformLayout> Create(
			const std::vector<ShaderReflection::ReflectDescriptorSetBinding>& bindings);

		explicit UniformLayout(const std::vector<ShaderReflection::ReflectDescriptorSetBinding>& bindings);
		virtual ~UniformLayout() = default;
		UniformLayout(const UniformLayout&) = delete;
		UniformLayout& operator=(const UniformLayout&) = delete;

		std::optional<ShaderReflection::ReflectDescriptorSetBinding> GetBindingByLocation(const uint32_t location) const;
		
		std::optional<ShaderReflection::ReflectDescriptorSetBinding> GetBindingByName(const std::string& name) const;

		uint32_t GetBindingLocationByName(const std::string& name) const;

		const std::vector<ShaderReflection::ReflectDescriptorSetBinding>& GetBindings() const { return m_Bindings; }

	protected:
		std::vector<ShaderReflection::ReflectDescriptorSetBinding> m_Bindings;
	};

}
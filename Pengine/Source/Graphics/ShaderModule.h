#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

#include "../Graphics/ShaderReflection.h"

namespace Pengine
{

	class PENGINE_API ShaderModule : public Asset
	{
	public:
		enum class Type
		{
			VERTEX,
			GEOMETRY,
			FRAGMENT,
			COMPUTE
		};

		static std::shared_ptr<ShaderModule> Create(
			const std::filesystem::path& filepath,
			const Type type);

		ShaderModule(
			const std::filesystem::path& filepath,
			const Type type);
		virtual ~ShaderModule() = default;
		ShaderModule(const ShaderModule&) = delete;
		ShaderModule& operator=(const ShaderModule&) = delete;

		virtual void Reload(bool useCache = true) = 0;

		virtual bool IsValid() const = 0;

		Type GetType() const { return m_Type; }

		const ShaderReflection::ReflectShaderModule GetReflection() const { return m_Reflection; }

	protected:
		ShaderReflection::ReflectShaderModule m_Reflection{};

		Type m_Type{};
	};

}

#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API UniformLayout
	{
	public:
		enum class Stage
		{
			VERTEX,
			FRAGMENT
		};

		enum class Type
		{
			SAMPLER,
			SAMPLER_ARRAY,
			BUFFER
		};

		struct Variable
		{
			std::string name;
			std::string type;
			size_t offset = 0;
		};

		struct Binding
		{
			std::vector<Variable> values;
			std::vector<Stage> stages;
			std::string name;
			std::string info;
			
			/**
			* Used for sampler array for now.
			*/
			size_t count;
			
			Type type;

			std::optional<Variable> GetValue(const std::string& name);
		};

		static std::shared_ptr<UniformLayout> Create(
			const std::unordered_map<uint32_t, Binding>& bindings);

		UniformLayout(const std::unordered_map<uint32_t, Binding>& bindings);
		virtual ~UniformLayout() = default;
		UniformLayout(const UniformLayout&) = delete;
		UniformLayout& operator=(const UniformLayout&) = delete;

		Binding GetBindingByLocation(uint32_t location) const;

		Binding GetBindingByName(const std::string& name) const;

		uint32_t GetBindingLocationByName(const std::string& name) const;

		const std::unordered_map<uint32_t, Binding>& GetBindingsByLocation() { return m_BindingsByLocation; };

		const std::unordered_map<std::string, uint32_t>& GetBindingLocationsByName() { return m_BindingLocationsByName; };

	protected:
		std::unordered_map<uint32_t, Binding> m_BindingsByLocation;
		std::unordered_map<std::string, uint32_t> m_BindingLocationsByName;
	};

}
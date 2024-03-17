#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API UUID
	{
	public:
		std::string Generate();

		UUID()
		{
			Generate();
		}

		UUID(std::string uuid)
			: m_UUID(std::move(uuid))
		{}

		UUID(const UUID& uuid) = default;

		UUID(UUID&& uuid) noexcept
		{
			m_UUID = std::move(uuid.m_UUID);
			uuid.m_UUID.clear();
		}

		~UUID() = default;

		UUID& operator=(const UUID& uuid);
		UUID& operator=(UUID&& uuid) noexcept;

		operator const std::string&() const { return m_UUID; }
		[[nodiscard]] const std::string& Get() const { return m_UUID; }

	private:
		std::string m_UUID;
	};

}
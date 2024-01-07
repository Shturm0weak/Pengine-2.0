#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API UUID
	{
	private:

		std::string m_UUID;
	public:

		std::string Generate();

		UUID()
		{
			Generate();
		}

		UUID(const std::string& uuid)
			: m_UUID(uuid)
		{}

		UUID(const UUID& uuid)
			: m_UUID(uuid.m_UUID)
		{}

		UUID(UUID&& uuid) noexcept
		{
			m_UUID = std::move(uuid.m_UUID);
			uuid.m_UUID.clear();
		}

		~UUID() = default;

		void operator=(const UUID& uuid) { m_UUID = uuid.m_UUID; }
		void operator=(UUID&& uuid) noexcept { m_UUID = uuid.m_UUID; }

		operator const std::string&() const { return m_UUID; }
		const std::string& Get() const { return m_UUID; }
	};

}
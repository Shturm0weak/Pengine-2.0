#pragma once

#include <random>

namespace Pengine
{

	class UUID
	{
	public:
		static uint64_t Generate();

		static UUID FromString(const std::string& string);

		UUID();

		UUID(uint64_t upper, uint64_t lower)
			: m_Upper(std::move(upper))
			, m_Lower(std::move(lower))
		{}

		UUID(const UUID& uuid)
		{
			m_Upper = uuid.GetUpper();
			m_Lower = uuid.GetLower();
		}

		UUID(UUID&& uuid) noexcept
		{
			m_Upper = std::move(uuid.m_Upper);
			m_Lower = std::move(uuid.m_Lower);
			uuid.m_Upper = 0;
			uuid.m_Lower = 0;
		}

		~UUID() = default;

		UUID& operator=(const UUID& uuid);
		UUID& operator=(UUID&& uuid) noexcept;

		[[nodiscard]] uint64_t GetUpper() const { return m_Upper; }
		[[nodiscard]] uint64_t GetLower() const { return m_Lower; }

		[[nodiscard]] bool IsValid() const { return m_Upper > 0 && m_Lower > 0; }

		[[nodiscard]] std::string ToString() const;

		bool operator==(const UUID& uuid) const { return m_Upper == uuid.m_Upper && m_Lower == uuid.m_Lower; }

	private:
		uint64_t m_Upper = 0;
		uint64_t m_Lower = 0;

		class NibblePacker
		{
		private:
			uint64_t m_Packed = 0;
			int m_Position = 0;

		public:
			enum class BitsType
			{
				Upper,
				Lower,
			};

			void Reset();

			void Pack(uint8_t byte, BitsType bitsType = BitsType::Lower);

			uint64_t Get() const { return m_Packed; }

			int Remaining() const { return (64 - m_Position) / 4; }
		};
	};

	struct uuid_hash
	{
		std::size_t operator()(const UUID& uuid) const
		{
			const uint64_t a = uuid.GetUpper();
			const uint64_t b = uuid.GetLower();
			return ((a + b) / 2 * (a + b + 1)) + b;
		}
	};

}

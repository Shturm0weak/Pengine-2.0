#include "UUID.h"

#include "RandomGenerator.h"

#include "../Utils/Utils.h"

namespace Pengine
{
	uint64_t UUID::Generate()
	{
		RandomGenerator& random = RandomGenerator::GetInstance();
		NibblePacker packer;

		for (size_t i = 0; i < 16; i++)
		{
			packer.Pack(random.Get<short>(0, 15));
		}

		return packer.Get();
	}

	UUID UUID::FromString(const std::string& string)
	{
		assert(string.size() == 34, "UUID string have to have two hex uint64_t, each one has 16 digits!");

		// The first characters are 0x

		const std::string upperUUIDString = string.substr(2, 16);
		const std::string lowerUUIDString = string.substr(18, 16);
		const uint64_t upper = Utils::stringToUint64(upperUUIDString, 16);
		const uint64_t lower = Utils::stringToUint64(lowerUUIDString, 16);

		return UUID(upper, lower);
	}

	UUID::UUID()
	{
		m_Upper = Generate();
		m_Lower = Generate();
	}

	UUID& UUID::operator=(const UUID& uuid)
	{
		if (this != &uuid)
		{
			m_Upper = uuid.m_Upper;
			m_Lower = uuid.m_Lower;
		}

		return *this;
	}

	UUID& UUID::operator=(UUID&& uuid) noexcept
	{
		m_Upper = std::move(uuid.m_Upper);
		m_Lower = std::move(uuid.m_Lower);
		uuid.m_Upper = 0;
		uuid.m_Lower = 0;
		return *this;
	}

	std::string UUID::ToString() const
	{
		std::stringstream stringstream;
		stringstream << "0x"
			<< std::hex << std::setw(16) << std::setfill('0')
			<< std::nouppercase
			<< static_cast<unsigned long long>(GetUpper())
			<< std::hex << std::setw(16) << std::setfill('0')
			<< std::nouppercase
			<< static_cast<unsigned long long>(GetLower());

		std::string string = stringstream.str();

		assert(string.size() == 34, "UUID string have to have two hex uint64_t, each one has 16 digits!");

		return string;
	}

	void UUID::NibblePacker::Reset()
	{
		m_Packed = 0;
		m_Position = 0;
	}

	void UUID::NibblePacker::Pack(uint8_t byte, BitsType bitsType)
	{
			if (m_Position >= 64) return;

			uint8_t nibble = bitsType == BitsType::Upper ?
				(byte >> 4) & 0x0F :  // Upper 4 bits
				byte & 0x0F;          // Lower 4 bits

			m_Packed |= static_cast<uint64_t>(nibble) << (60 - m_Position);
			m_Position += 4;
	}

}

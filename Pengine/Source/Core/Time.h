#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API Time
	{
	public:
		static const std::string& GetDate() { return GetInstance().m_CurrentDate; }

		static const double GetTime() { return GetInstance().m_GlobalTime; }

		static const double GetDeltaTime() { return GetInstance().m_DeltaTime; }

	private:
		Time();
		~Time() = default;
		Time(const Time&) = delete;
		Time& operator=(const Time&) = delete;

		static Time& GetInstance();

		void Update();

		std::string m_CurrentDate;
		double m_DeltaTime;
		double m_GlobalTime;
		double m_LastTime;

		friend class EntryPoint;
	};

}
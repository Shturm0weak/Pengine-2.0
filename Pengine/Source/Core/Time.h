#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API Time
	{
	public:
		static const std::string& GetDate() { return GetInstance().m_CurrentDate; }

		static double GetTime() { return GetInstance().m_GlobalTime; }

		static double GetDeltaTime() { return GetInstance().m_DeltaTime; }

		Time(const Time&) = delete;
		Time& operator=(const Time&) = delete;

	private:
		Time();
		~Time() = default;

		static Time& GetInstance();

		void Update();

		std::string m_CurrentDate;
		double m_DeltaTime = 0.0;
		double m_GlobalTime = 0.0;
		double m_LastTime = 0.0;

		friend class EntryPoint;
	};

}
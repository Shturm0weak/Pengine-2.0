#pragma once

#include "Core.h"

#include <chrono>

namespace Pengine
{

	class PENGINE_API Timer 
	{
	public:
		explicit Timer(const std::string& name = "Timer", bool showTime = true, double* outTime = nullptr);

		~Timer();

		float Stop() const;
	private:

		double* m_OutTime = nullptr;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimePoint;

		bool m_ShowTime = false;

		std::string m_Name;
	};

}

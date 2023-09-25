#pragma once

#include "Core.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <functional>

namespace Pengine
{

	class PENGINE_API Timer 
	{
	private:

		double* m_OutTime = nullptr;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimePoint;
	
		bool m_ShowTime = false;

	public:
		Timer(bool showTime = true, double* outTime = nullptr);

		~Timer();

		void Stop();
	};

}
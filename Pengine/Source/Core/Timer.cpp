#include "Timer.h"

#include <iomanip>

using namespace Pengine;

Timer::Timer(const bool showTime, double* outTime)
	: m_OutTime(outTime)
	, m_ShowTime(showTime)
{
	m_StartTimePoint = std::chrono::high_resolution_clock::now();
}

Timer::~Timer()
{
	Stop();
}

void Timer::Stop() const
{
	const auto endTimePoint = std::chrono::high_resolution_clock::now();
	const auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimePoint).time_since_epoch().count();
	const auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint).time_since_epoch().count();
	const auto duration = end - start;
	const double ms = static_cast<double>(duration) * 0.001;

	if (m_ShowTime)
	{
		std::setprecision(6);
		std::cout << duration << "us (" << ms << "ms)\n";
	}

	if (m_OutTime)
	{
		*m_OutTime = ms;
	}
}

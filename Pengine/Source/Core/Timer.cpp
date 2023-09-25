#include "Timer.h"

using namespace Pengine;

Timer::Timer(bool showTime, double* outTime) : m_ShowTime(showTime), m_OutTime(outTime)
{
	m_StartTimePoint = std::chrono::high_resolution_clock::now();
}

Timer::~Timer()
{
	Stop();
}

void Timer::Stop()
{
	auto endTimePoint = std::chrono::high_resolution_clock::now();
	auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimePoint).time_since_epoch().count();
	auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint).time_since_epoch().count();
	auto duration = end - start;
	double ms = duration * 0.001;

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
#include "Time.h"

#include <chrono>

using namespace Pengine;

Time::Time()
{
	Update();
}

Time& Time::GetInstance()
{
	static Time time;
	return time;
}

void Time::Update()
{
	const std::time_t time = std::time(0);
	std::string date = ctime(&time);
	date = date.substr(0, date.size() - 1);
	m_CurrentDate = std::move(date);

	const double duration = std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	m_DeltaTime = fabs(m_LastTime - duration);
	m_LastTime = duration;
	m_GlobalTime += m_DeltaTime;
}

#include "Logger.h"

#include "Time.h"

using namespace Pengine;

constexpr char const* logFilepath = "Log.txt";

void Logger::Log(const std::string& message, const char* color)
{
	GetInstance().m_OutFile << "[" << Time::GetDate() << "] " << message << std::endl;
	std::cout << color << message << RESET << std::endl;
}

void Logger::Warning(const std::string& message)
{
	GetInstance().m_OutFile << "[" << Time::GetDate() << "] WARNING:" << message << std::endl;
	std::cout << YELLOW << "WARNING:" << message << RESET << std::endl;
}

void Logger::Error(const std::string& message)
{
	GetInstance().m_OutFile << "[" << Time::GetDate() << "] ERROR:" << message << std::endl;
	std::cout << RED << "ERROR:" << message << RESET << std::endl;
}

void Logger::FatalError(const std::string& message)
{
	GetInstance().m_OutFile << "[" << Time::GetDate() << "] FATAL_ERROR:" << message << std::endl;
	std::cout << RED << "FATAL_ERROR:" << message << RESET << std::endl;
	GetInstance().m_OutFile.close();
	throw std::runtime_error(message);
}

Logger::Logger()
{
	m_OutFile.open(logFilepath);
}

Logger::~Logger()
{
	m_OutFile.close();
}

Logger& Logger::GetInstance()
{
	static Logger logger;
	return logger;
}

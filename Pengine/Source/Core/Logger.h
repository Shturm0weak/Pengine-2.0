#pragma once

#include "Core.h"
#include "ColoredOutput.h"

#include <fstream>
#include <mutex>

namespace Pengine
{

	class PENGINE_API Logger
	{
	public:
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		static void Log(const std::string& message, const char* color = RESET);

		static void Warning(const std::string& message);

		static void Error(const std::string& message);

		static void FatalError(const std::string& message);

	private:
		Logger();
		~Logger();

		static Logger& GetInstance();

		std::ofstream m_OutFile;
		std::mutex m_Mutex;
	};

#define FATAL_ERROR(message) Logger::FatalError(std::string(message) + " At: " + __FILE__ + " " + std::to_string(__LINE__)); throw std::runtime_error(message)

}

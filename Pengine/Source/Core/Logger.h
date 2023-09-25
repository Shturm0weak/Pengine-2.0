#pragma once

#include "Core.h"
#include "ColoredOutput.h"

#include <fstream>

namespace Pengine
{

	class PENGINE_API Logger
	{
	public:
		static void Log(const std::string& message, const char* color = RESET);

		static void Warning(const std::string& message);

		static void Error(const std::string& message);

		static void FatalError(const std::string& message);

	private:
		Logger();
		~Logger();
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		static Logger& GetInstance();

		std::ofstream m_OutFile;
	};

#define FATAL_ERROR(message) Logger::FatalError(std::string(message) + " At: " + __FILE__ + " " + std::to_string(__LINE__));

}
#pragma once

#include <iostream>

#define DEBUG_INFO
#define DEBUG_WARNING
#define DEBUG_ERROR

namespace PPK
{
	class Logger
	{
	private:
		static inline int logCounter = 0;

		static inline void Print(const char* logPrefix, const char* message)
		{
			std::string log = logPrefix;
			log += "(" + std::to_string(logCounter) + ") " + message + "\n";
			OutputDebugStringA(log.c_str());
			logCounter++;
		}

	public:
		static inline void Info(const char* message)
		{
#ifdef DEBUG_INFO
			const char* logPrefix = "---- [PPK Engine] INFO: ";
			Print(logPrefix, message);
#endif
		}

		static inline void Warning(const char* message)
		{
#ifdef DEBUG_WARNING
			const char* logPrefix = "---- [PPK Engine] WARNING: ";
			Print(logPrefix, message);
#endif
		}

		static inline void Error(const char* message)
		{
#ifdef DEBUG_ERROR
			const char* logPrefix = "---- [PPK Engine] ERROR: ";
			Print(logPrefix, message);
			throw;
#endif
		}
	};
}
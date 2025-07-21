#pragma once

#define DEBUG_INFO
#define DEBUG_WARNING
#define DEBUG_ERROR

#include <string>
#include <debugapi.h>

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

		static inline void Print(const wchar_t* logPrefix, const wchar_t* message)
		{
			std::wstring log = logPrefix;
			log += L"(" + std::to_wstring(logCounter) + L") " + message + L"\n";
			OutputDebugStringW(log.c_str());
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

		static inline void Info(const wchar_t* message)
		{
#ifdef DEBUG_INFO
			const wchar_t* logPrefix = L"---- [PPK Engine] INFO: ";
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

		static inline void Warning(const wchar_t* message)
		{
#ifdef DEBUG_WARNING
			const wchar_t* logPrefix = L"---- [PPK Engine] WARNING: ";
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

		static inline void Error(const wchar_t* message)
		{
#ifdef DEBUG_ERROR
			const wchar_t* logPrefix = L"---- [PPK Engine] ERROR: ";
			Print(logPrefix, message);
			throw;
#endif
		}

		static inline void Assert(bool condition, const wchar_t* message = L"")
		{
#ifdef DEBUG_WARNING
			if (!condition)
			{
				Warning(message);
#if defined(_DEBUG)
				DebugBreak();
#endif
			}
#endif
		}

		static inline void Assert(bool condition, const char* message)
		{
#ifdef DEBUG_WARNING
			if (!condition)
			{
				Warning(message);
#if defined(_DEBUG)
				DebugBreak();
#endif
			}
#endif
		}
	};
}
#pragma once

#define LOG_VERBOSE 0

#include <string>
#include <debugapi.h>

namespace PPK
{
	class Logger
	{
	private:
		static inline int logCounter = 0;

		static void Print(const char* logPrefix, const char* message)
		{
			std::string log = logPrefix;
			log += "(" + std::to_string(logCounter) + ") " + message + "\n";
			OutputDebugStringA(log.c_str());
			logCounter++;
		}

		static void Print(const wchar_t* logPrefix, const wchar_t* message)
		{
			std::wstring log = logPrefix;
			log += L"(" + std::to_wstring(logCounter) + L") " + message + L"\n";
			OutputDebugStringW(log.c_str());
			logCounter++;
		}

	public:
		static void Verbose(const char* message)
		{
#if LOG_VERBOSE
			const char* logPrefix = "---- [PPK Engine] VERBOSE: ";
			Print(logPrefix, message);
#endif
		}

		static void Verbose(const wchar_t* message)
		{
#if LOG_VERBOSE
			const wchar_t* logPrefix = L"---- [PPK Engine] VERBOSE: ";
			Print(logPrefix, message);
#endif
		}

		static void Info(const char* message)
		{
#ifdef PPK_LOG_INFO
			const char* logPrefix = "---- [PPK Engine] INFO: ";
			Print(logPrefix, message);
#endif
		}

		static void Info(const wchar_t* message)
		{
#ifdef PPK_LOG_INFO
			const wchar_t* logPrefix = L"---- [PPK Engine] INFO: ";
			Print(logPrefix, message);
#endif
		}

		static void Warning(const char* message)
		{
#ifdef PPK_LOG_WARNING
					const char* logPrefix = "---- [PPK Engine] WARNING: ";
			Print(logPrefix, message);
#endif
		}

		static void Warning(const wchar_t* message)
		{
#ifdef PPK_LOG_WARNING
			const wchar_t* logPrefix = L"---- [PPK Engine] WARNING: ";
			Print(logPrefix, message);
#endif
		}

		static void Error(const char* message)
		{
#ifdef PPK_LOG_ERROR
			const char* logPrefix = "---- [PPK Engine] ERROR: ";
			Print(logPrefix, message);
			throw;
#endif
		}

		static void Error(const wchar_t* message)
		{
#ifdef PPK_LOG_ERROR
			const wchar_t* logPrefix = L"---- [PPK Engine] ERROR: ";
			Print(logPrefix, message);
			throw;
#endif
		}

		static void Assert(bool condition, const wchar_t* message = L"")
		{
#ifdef PPK_LOG_WARNING
			if (!condition) [[unlikely]]
			{
				Warning(message);
#ifdef PPK_BREAK_ON_WARNING
				DebugBreak();
#endif
			}
#endif
		}

		static void Assert(bool condition, const char* message)
		{
#ifdef PPK_LOG_WARNING
			if (!condition) [[unlikely]]
			{
				Warning(message);
#ifdef PPK_BREAK_ON_WARNING
				DebugBreak();
#endif
			}
#endif
		}
	};
}
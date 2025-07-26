#pragma once

#define DEBUG_VERBOSE 0
#define DEBUG_INFO 1
#define DEBUG_WARNING 1
#define DEBUG_ERROR 1

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
#if DEBUG_VERBOSE
			const char* logPrefix = "---- [PPK Engine] VERBOSE: ";
			Print(logPrefix, message);
#endif
		}

		static void Verbose(const wchar_t* message)
		{
#if DEBUG_VERBOSE
			const wchar_t* logPrefix = L"---- [PPK Engine] VERBOSE: ";
			Print(logPrefix, message);
#endif
		}

		static void Info(const char* message)
		{
#if DEBUG_INFO
			const char* logPrefix = "---- [PPK Engine] INFO: ";
			Print(logPrefix, message);
#endif
		}

		static void Info(const wchar_t* message)
		{
#if DEBUG_INFO
			const wchar_t* logPrefix = L"---- [PPK Engine] INFO: ";
			Print(logPrefix, message);
#endif
		}

		static void Warning(const char* message)
		{
#if DEBUG_WARNING
					const char* logPrefix = "---- [PPK Engine] WARNING: ";
			Print(logPrefix, message);
#endif
		}

		static void Warning(const wchar_t* message)
		{
#if DEBUG_WARNING
			const wchar_t* logPrefix = L"---- [PPK Engine] WARNING: ";
			Print(logPrefix, message);
#endif
		}

		static void Error(const char* message)
		{
#if DEBUG_ERROR
			const char* logPrefix = "---- [PPK Engine] ERROR: ";
			Print(logPrefix, message);
			throw;
#endif
		}

		static void Error(const wchar_t* message)
		{
#if DEBUG_ERROR
			const wchar_t* logPrefix = L"---- [PPK Engine] ERROR: ";
			Print(logPrefix, message);
			throw;
#endif
		}

		static void Assert(bool condition, const wchar_t* message = L"")
		{
#if DEBUG_WARNING
			if (!condition) [[unlikely]]
			{
				Warning(message);
#if defined(_DEBUG)
				DebugBreak();
#endif
			}
#endif
		}

		static void Assert(bool condition, const char* message)
		{
#if DEBUG_WARNING
			if (!condition) [[unlikely]]
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
#pragma once

#include <chrono>
#include <Logger.h>
#include <string>

namespace PPK
{
	class Timer
	{
	public:
		inline static void BeginTimer()
		{
			m_timerStart = std::chrono::high_resolution_clock::now();
		}

		inline static void EndTimer()
		{
			m_timerEnd = std::chrono::high_resolution_clock::now();
		}

		inline static void EndAndReportTimer(std::string message)
		{
			EndTimer();
			float ms = std::chrono::duration_cast<std::chrono::microseconds>(m_timerEnd - m_timerStart).count();
			Logger::Info((message + ": " + std::to_string(ms) + std::string(" us")).c_str());
		}

		inline static float GetApplicationTimeInMilliseconds()
		{
			return GetCurrentTime() - initialTime;
		}

		inline static double GetApplicationTimeInSeconds()
		{
			return (GetCurrentTime() - initialTime) / static_cast<double>(1000);
		}

		inline static double initialTime = GetCurrentTime();
		inline static std::chrono::time_point<std::chrono::high_resolution_clock> m_timerStart{};
		inline static std::chrono::time_point<std::chrono::high_resolution_clock> m_timerEnd{};
	};
}

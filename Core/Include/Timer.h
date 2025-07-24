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

// 2k for screenshots
#define NUM_ITERATIONS_AVERAGED 100
	struct RingBufferFltIter
	{
		std::array<float, NUM_ITERATIONS_AVERAGED> m_elements;
		uint32_t m_currentIndex;

		RingBufferFltIter()
		{
			m_elements.fill(0.f);
			m_currentIndex = 0u;
		}

		void AddElement(float newElement)
		{
			m_elements[m_currentIndex] = newElement;
			m_currentIndex = (m_currentIndex + 1) % m_elements.size();
		}

		[[nodiscard]] float GetAverage() const
		{
			float totalSum = 0.f;
			for (auto element : m_elements)
			{
				totalSum += element;
			}
			return totalSum / static_cast<float>(m_elements.size());
		}
	};

	inline std::unordered_map<std::string, RingBufferFltIter> gTimePerScope;

#define PROFILING_ENABLED 1
#if PROFILING_ENABLED
#define SCOPED_TIMER(scopeName) ScopedTimer scopedTimer(scopeName);
// Hacky way to prevent variable overlap if scopes are nested
#define SCOPED_TIMER_1(scopeName) ScopedTimer scopedTimer1(scopeName);
#define SCOPED_TIMER_2(scopeName) ScopedTimer scopedTimer2(scopeName);
#else
#define SCOPED_TIMER(scopeName) 
#define SCOPED_TIMER_1(scopeName) 
#define SCOPED_TIMER_2(scopeName) 
#endif

	class ScopedTimer
	{
	public:
		ScopedTimer(std::string scopeName)
		: m_scopeName(scopeName), m_startTime(std::chrono::high_resolution_clock::now())
		{
		}

		~ScopedTimer()
		{
			if (!gTimePerScope.contains(m_scopeName))
			{
				gTimePerScope[m_scopeName] = {};
			}

			float scopeTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_startTime).count();
			gTimePerScope[m_scopeName].AddElement(scopeTime);
		}

		std::string m_scopeName;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime{};
	};
}

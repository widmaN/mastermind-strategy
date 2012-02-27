/// @defgroup HRTimer High-Resolution Timer
/// @ingroup util

#ifndef UTILITIES_HR_TIMER_HPP
#define UTILITIES_HR_TIMER_HPP

#define HAS_CHRONO 1

#if HAS_CHRONO
#include <chrono>
#endif

namespace util
{

/// High-resolution timer used to time program execution.
/// @ingroup HRTimer
class hr_timer
{
#if HAS_CHRONO
	typedef std::chrono::high_resolution_clock clock;
	clock::time_point m_start;
#endif

public:

	/// Creates a timer.
	hr_timer() { }

	/// Starts timing.
	void start()
	{
#if HAS_CHRONO
		m_start = clock::now();
#endif
	}

	/// Stops timing and returns the number of seconds elapsed
	/// since the last call to <code>start()</code>.
	double stop()
	{
#if HAS_CHRONO
		std::chrono::duration<double> t1 = clock::now() - m_start;
		return t1.count();
#else
		return 0.0;
#endif
	}
};

} // namespace util

#endif // UTILITIES_HR_TIMER_HPP

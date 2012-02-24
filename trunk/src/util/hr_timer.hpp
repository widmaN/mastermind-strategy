/// @defgroup HRTimer High-Resolution Timer
/// @ingroup util

#ifndef UTILITIES_HR_TIMER_HPP
#define UTILITIES_HR_TIMER_HPP

#include <chrono>

namespace util
{
	
/// High-resolution timer used to time program execution.
/// @ingroup HRTimer
class hr_timer 
{
	typedef std::chrono::high_resolution_clock clock;
	clock::time_point m_start;

public:

	/// Creates a timer.
	hr_timer() { }

	/// Starts timing.
	void start()
	{
		m_start = clock::now();
	}

	/// Stops timing and returns the number of seconds elapsed
	/// since the last call to <code>start()</code>.
	double stop()
	{
		std::chrono::duration<double> t1 = clock::now() - m_start;
		return t1.count();
	}
};

} // namespace util

#endif // UTILITIES_HR_TIMER_HPP

#ifndef MASTERMIND_HRTIMER_H
#define MASTERMIND_HRTIMER_H

#include <chrono>

/// Contains helper classes.
namespace Utilities
{
	
/// High-resolution timer used to time program execution.
class HRTimer 
{
	typedef std::chrono::high_resolution_clock clock;
	clock::time_point m_start;

public:

	HRTimer() { }

	/// Starts timing.
	void start()
	{
		m_start = clock::now();
	}

	/// Stops timing and returns the number of seconds elapsed.
	double stop()
	{
		std::chrono::duration<double> t1 = clock::now() - m_start;
		return t1.count();
	}
};

} // namespace Utilities


#endif // MASTERMIND_HRTIMER_H

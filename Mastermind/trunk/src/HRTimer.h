#pragma once

/// Contains helper classes.
namespace Utilities
{
	/// High-resolution timer used to time program execution.
	class HRTimer 
	{
	private:
		__int64 m_start;
		__int64 m_stop;
		double m_frequency;
		double m_time;

	public:
		static double GetFrequency();

	public:
		HRTimer();
		//void Reset();

		/// Starts the timer.
		void Start();

		//void Pause();
		//void Resume();

		/// Stops the timer and returns the number of seconds elapsed.
		double Stop();
	};
}
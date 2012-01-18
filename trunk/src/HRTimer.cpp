#include <Windows.h>

#include "HRTimer.h"

using namespace Utilities;

HRTimer::HRTimer()
{
	m_frequency = HRTimer::GetFrequency();
}

double HRTimer::GetFrequency()
{
	LARGE_INTEGER proc_freq;
	QueryPerformanceFrequency(&proc_freq);
	return (double)proc_freq.QuadPart;
}

void HRTimer::Start()
{
	LARGE_INTEGER t;
	DWORD_PTR oldmask = ::SetThreadAffinityMask(::GetCurrentThread(), 0);
	::QueryPerformanceCounter(&t);
	::SetThreadAffinityMask(::GetCurrentThread(), oldmask);
	m_start = t.QuadPart;
}

double HRTimer::Stop()
{
	LARGE_INTEGER t;
	DWORD_PTR oldmask = ::SetThreadAffinityMask(::GetCurrentThread(), 0);
	::QueryPerformanceCounter(&t);
	::SetThreadAffinityMask(::GetCurrentThread(), oldmask);
	m_stop = t.QuadPart;
	return ((m_stop - m_start) / m_frequency);
}

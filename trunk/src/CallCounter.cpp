//#include <stdio.h>
//#include <memory.h>
//#include <string.h>
//#include <emmintrin.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#ifdef _WIN32
#include <intrin.h>
#endif

#include "CallCounter.h"

using namespace Utilities;

CallCounter::CallCounter(const char *name, bool enabled) : m_name(name)
{
	m_enabled = enabled;
	Reset();
}

void CallCounter::Enable(bool enabled)
{
	m_enabled = enabled;
}

bool CallCounter::IsEnabled() const
{
	return m_enabled;
}

void CallCounter::Reset()
{
	m_calls = 0;
	m_comps = 0;
	memset(m_callstat, 0, sizeof(m_callstat));
	memset(m_compstat, 0, sizeof(m_compstat));
}

#if 1
std::ostream& operator << (std::ostream &os, const CallCounter &cc)
{
	os << "==== Call Statistics for " << cc.name() << "() ====" << std::endl;
	os << "Total # of calls: " << cc.calls() << std::endl;
	os << "Total # of computations: " << cc.computations() << std::endl;
	if (cc.calls() > 0)
	{
		double comps_per_call = (double)cc.computations() / cc.calls();
		os << "Average computations per call: " << comps_per_call << std::endl;
		os << "#Comp    %Call    %Comp" << std::endl;
		for (int k = 31; k >= 0; k--)
		{
			if (cc.m_callstat[k] > 0)
			{
				os << ">=" << std::setw(6) << (1<<k) << " : "
					<< std::setw(6) << std::setprecision(2)
					<< (double)cc.m_callstat[k] / cc.calls() * 100.0 << "  "
					<< std::setw(6) << std::setprecision(2)
					<< (double)cc.m_compstat[k] / cc.computations() * 100.0 << std::endl;
			}
		}
	}
	return os;
}
#else
void CallCounter::DebugPrint() const
{
	printf("==== Call Statistics for %s() ====\n", m_name);
	printf("Total # of calls: %u\n", m_calls);
	printf("Total # of computations: %u\n", m_comps);
	if (m_calls > 0) {
		printf("Average computations per call: %.2f\n", (double)m_comps / m_calls);
		printf("#Comp    %%Call    %%Comp\n");
		for (int k = 31; k >= 0; k--) {
			if (m_callstat[k] > 0) {
				printf(">=%6d : %6.2f%%  %6.2f%%\n", 1<<k,
					(double)m_callstat[k] / m_calls * 100.0,
					(double)m_compstat[k] / m_comps * 100.0);
			}
		}
	}
}
#endif

void CallCounter::AddCall(unsigned int comp)
{
	if (m_enabled && comp)
	{
		unsigned long pos;
#ifdef _WIN32
		_BitScanReverse(&pos, comp);
#else
		pos = 31 - __builtin_clz(comp);
#endif
		m_callstat[pos]++;
		m_compstat[pos] += comp;
		m_calls++;
		m_comps += comp;
	}
}

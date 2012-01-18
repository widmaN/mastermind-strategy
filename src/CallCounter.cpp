#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <intrin.h>

#include "CallCounter.h"

using namespace Utilities;

CallCounter::CallCounter(const char *name, bool enabled)
{
	strcpy_s(m_name, name);
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

void CallCounter::AddCall(unsigned int comp)
{
	if (m_enabled) {
		unsigned long pos;
		if (_BitScanReverse(&pos, comp)) {
			m_callstat[pos]++;
			m_compstat[pos] += comp;
			m_calls++;
			m_comps += comp;
		}
	}
}

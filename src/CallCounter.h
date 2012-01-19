#ifndef MASTERMIND_CALL_COUNTER_HPP
#define MASTERMIND_CALL_COUNTER_HPP

#include <string>
#include <ostream>

namespace Utilities
{

class CallCounter
{
	std::string m_name;
	unsigned int m_calls;
	unsigned int m_comps;
	bool m_enabled;

public:
	unsigned int m_callstat[32];
	unsigned int m_compstat[32];

public:
	CallCounter(const char *name, bool enabled);
	void Enable(bool enabled);
	bool IsEnabled() const;
	void Reset();
	void AddCall(unsigned int comp);
	void DebugPrint() const;

	unsigned int calls() const { return m_calls; }
	unsigned int computations() const { return m_comps; }
	std::string name() const { return m_name; }

};

std::ostream& operator << (std::ostream &os, const CallCounter &cc);

} // namespace Utilities

#endif // MASTERMIND_CALL_COUNTER_HPP

#pragma once

namespace Utilities
{
	class CallCounter
	{
	private:
		char m_name[100];
		unsigned int m_callstat[32];
		unsigned int m_compstat[32];
		unsigned int m_calls;
		unsigned int m_comps;

	public:
		CallCounter();
		CallCounter(const char *name);
		void Reset();
		void DebugPrint() const;
		void AddCall(unsigned int comp);
	};

}

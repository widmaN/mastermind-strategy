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
		bool m_enabled;

	public:
		CallCounter(const char *name, bool enabled);
		void Enable(bool enabled);
		bool IsEnabled() const;
		void Reset();
		void AddCall(unsigned int comp);
		void DebugPrint() const;
	};
}
